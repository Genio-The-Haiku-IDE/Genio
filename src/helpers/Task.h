/*
 * Copyright 2023, Nexus6 
 * All rights reserved. Distributed under the terms of the MIT license.
 */
 
#pragma once

#include <any>
#include <functional>
#include <map>
#include <stdexcept>

#include <Alignment.h>
#include <Message.h>
#include <Messenger.h>
#include <String.h>
#include <SupportDefs.h>

namespace Genio::Task {

	using namespace std;

	namespace Private {

		struct ThreadData {
			void *target_function;
			BMessenger *messenger;
			thread_id id;
			const char *name;
		};

		static exception_ptr current_exception_ptr;
		static map<thread_id, exception_ptr> TaskExceptionMap;
		static map<string, type_code> TypeMap = {
			{typeid(BAlignment).name(), B_ALIGNMENT_TYPE},
			{typeid(BRect).name(), B_RECT_TYPE},
			{typeid(BPoint).name(), B_POINT_TYPE},
			{typeid(BSize).name(), B_SIZE_TYPE},
			{typeid(const char *).name(), B_STRING_TYPE},
			{typeid(BString).name(), B_STRING_TYPE},
			{typeid(std::string).name(), B_STRING_TYPE},
			// {typeid(BStringList).name(), B_STRING_LIST_TYPE},
			{typeid(int8).name(), B_INT8_TYPE},
			{typeid(int16).name(), B_INT16_TYPE},
			{typeid(int32).name(), B_INT32_TYPE},
			{typeid(int64).name(), B_INT64_TYPE},
			{typeid(uint8).name(), B_UINT8_TYPE},
			{typeid(uint16).name(), B_UINT16_TYPE},
			{typeid(uint32).name(), B_UINT32_TYPE},
			{typeid(uint64).name(), B_UINT64_TYPE},
			{typeid(bool).name(), B_BOOL_TYPE},
			{typeid(float).name(), B_FLOAT_TYPE},
			{typeid(double).name(), B_DOUBLE_TYPE},
			{typeid(rgb_color).name(), B_RGB_COLOR_TYPE},
			{typeid(const void *).name(), B_POINTER_TYPE},
			{typeid(BMessenger).name(), B_MESSENGER_TYPE},
			{typeid(entry_ref).name(), B_REF_TYPE},
			{typeid(node_ref).name(), B_NODE_REF_TYPE},
			{typeid(BMessage).name(), B_MESSAGE_TYPE},
		};
			
		template <typename T>
		struct BMessageType {
			static type_code Get() { return TypeMap[typeid(T).name()]; }  
		};
	}

	using namespace Private;

	const int TASK_RESULT_MESSAGE = 'tfwr';
	
	template <typename ResultType>
	class Task;
	
	
	template <typename ResultType>
	class TaskResult: public BArchivable {
	public:
		
		TaskResult(const BMessage &archive) 
			: fResult(nullptr)
		{
			ssize_t size = 0;
			type_code type;
			void *result;
			
			if constexpr (std::is_void<ResultType>::value == false) {
				if (archive.GetInfo(fResultField, &type) == B_OK) {
					status_t error = archive.FindData(fResultField, type, (const void **)&result, &size);
					if (error == B_OK) {
						fResult = *(ResultType*)result;
						// OKAlert("", BString("result:") << *(ResultType*)result, B_INFO_ALERT);
						// OKAlert("", BString("fResult:") << any_cast<ResultType>(fResult) << " type:" << fResult.type().name(), B_INFO_ALERT);
					}
				}
			}
			
			if (archive.FindInt32(fTaskIdField, &fId) != B_OK) {
					throw runtime_error("Can't unarchive TaskResult instance: Task ID not available");
			}
			if (archive.FindString(fTaskNameField, &fName) != B_OK) {
					throw runtime_error("Can't unarchive TaskResult instance: Task Name not available");
			}
			
			// archive.PrintToStream();
		}
		
		~TaskResult() {}

		ResultType	GetResult() 
		{ 		
			auto ptr = TaskExceptionMap[fId];
			if (ptr) {
				rethrow_exception(ptr);
			} else {
				if constexpr (std::is_void<ResultType>::value == false) {
					return any_cast<ResultType>(fResult);
				}
			}
		}
		
		virtual status_t Archive(BMessage *archive, bool deep) const
		{
			status_t status;
			status = BArchivable::Archive(archive, deep);
			
			if constexpr (std::is_void<ResultType>::value == false) {
				if (fResult.has_value()) {
					type_code type = BMessageType<ResultType>::Get();
					ResultType result = any_cast<ResultType>(fResult);
					status = archive->AddData(fResultField, type, &result, sizeof(ResultType));
				}
			}
			status = archive->AddInt32(fTaskIdField, fId);
			status = archive->AddString(fTaskNameField, fName);
			
			status = archive->AddString("class", "TaskResult");
			// archive->PrintToStream();
			return status;
		}
		
		static TaskResult<ResultType>* Instantiate(BMessage* archive)
		{
			if (validate_instantiation(archive, "TaskResult")) {
				return new TaskResult<ResultType>(*archive);
			} else {
				return nullptr;
			}
		}
		
		thread_id GetTaskID() { return fId; }
		const char* GetTaskName() { return fName; }
		
	private:
		friend class	Task<ResultType>;
		
		const char*		fResultField = "TaskResult::Result";
		const char*		fTaskIdField = "TaskResult::TaskID";
		const char*		fTaskNameField = "TaskResult::TaskName";
		
		any				fResult;
		thread_id		fId;
		const char *	fName;
		
						TaskResult() {}
	};
	
	
	template <typename ResultType>
	class Task {
	public:
		using native_handle_type = thread_id;

		template<typename Function, typename... Args>
		Task(const char *name, BMessenger *messenger, Function&& function, Args&&... args)
		{
			auto *target_function =
				new arguments_wrapper<Function, Args...>(std::forward<Function>(function),
				std::forward<Args>(args)...);
				
			using lambda_type = std::remove_reference_t<decltype(*target_function)>;

			ThreadData *thread_data = new ThreadData;
			thread_data->target_function = target_function;
			thread_data->messenger = messenger;
			thread_data->name = name;
			
			thread_handle = spawn_thread(&_CallTarget<ThreadData, lambda_type>,
										name,
										B_NORMAL_PRIORITY,
										thread_data);
										
			thread_data->id = thread_handle;
										
			if (thread_handle == 0) {
				delete target_function;
				delete thread_data;
				throw runtime_error("Can't create task");
			}
		}
		
		~Task() {}
		
		status_t Run() 
		{ 
			return resume_thread(thread_handle);
		}
								
		status_t Stop() 
		{ 
			return kill_thread(thread_handle);
		}					
		
	private:
		native_handle_type thread_handle;
		
		template<typename Data, typename Lambda>
		static int _CallTarget(void *target)
		{
			TaskResult<ResultType> task_result;
			BMessage msg(TASK_RESULT_MESSAGE);
			Data *data;
			Lambda *lambda;
			BMessenger *messenger;
			
			try {
				data = reinterpret_cast<Data *>(target); 
				lambda = reinterpret_cast<Lambda *>(data->target_function);
				messenger = data->messenger;
				task_result.fId = data->id;
				task_result.fName = data->name;
				
				using ret_t = decltype((*lambda)());
				
				if constexpr (std::is_same_v<void, ret_t>) {
					(*lambda)();
				} else {
					auto result = (*lambda)();
					task_result.fResult = result;
				}
			} catch(...) {
				TaskExceptionMap[task_result.fId] = current_exception();
			}
			
			delete lambda;
			delete data;
			
			if (task_result.Archive(&msg, false) == B_OK)
			{
				if (messenger->IsValid()) {
					messenger->SendMessage(&msg);
				}
			} else {
				throw runtime_error("Can't create TaskResult message");
			}

			return B_OK;
		}
		
		template <typename Function, typename ... Args>
		class arguments_wrapper
		{
			Function callable;
			std::tuple<std::decay_t<Args>...> args;
		public:
			arguments_wrapper(Function&& callable, Args&& ... args) :
				callable(std::forward<Function>(callable)),
				args{std::forward<Args>(args)...}
			{}

			constexpr decltype(auto) operator()()
			{
				return apply(std::make_index_sequence<sizeof...(Args)>{});
			}
			
		private:
			template <std::size_t ... indices>
			constexpr decltype(auto) apply(std::index_sequence<indices...>)
			{
				return callable(std::move(std::get<indices>(args))...);
			}

		};
	};

}