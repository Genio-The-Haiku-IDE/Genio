/*
 * Copyright 2023, Nexus6 
 * All rights reserved. Distributed under the terms of the MIT license.
 */
 
#pragma once

#include <stdexcept>
#include <functional>
#include <map>

#include "Utils.h"

#include <String.h>
#include <Messenger.h>
#include <SupportDefs.h>

namespace Genio::Task {

	using namespace std;

	namespace Private {

		struct ThreadData {
			void *target_function;
			BMessenger *messenger;
			thread_id id;
		};

		static exception_ptr current_exception_ptr;
		static map<thread_id, exception_ptr> TaskExceptionMap;

		template <typename int32>
		struct BMessageType{ static type_code Get() { return B_INT32_TYPE; }  };
	}

	using namespace Private;

	const int TASK_RESULT_MESSAGE = 'tfwr';
	
	template <typename ResultType>
	class Task;
	
	class TMessage : public BMessage {
		public:
										TMessage() : BMessage() {};
										TMessage(uint32 what) : BMessage(what) {};
										TMessage(const BMessage& other) : BMessage(other) {};
			virtual						~TMessage();
		
			template <typename Type>
			status_t			Add(const char* name, Type value) {
									if (typeid(Type) == typeid(const BAlignment&))
										return AddAlignment(name, value); 
									if (typeid(Type) == typeid(BRect))
										return AddRect(name, value); 
									if (typeid(Type) == typeid(BPoint))
										return AddPoint(name, value); 
									if (typeid(Type) == typeid(BSize))
										return AddSize(name, value); 
									if (typeid(Type) == typeid(const char*))
										return AddString(name, value);
									if (typeid(Type) == typeid(const BString&))
										return AddString(name, value); 
									// if (typeid(Type) == typeid(const BStringList&))
										// return AddStrings(name, value); 
									if (typeid(Type) == typeid(int8))
										return AddInt8(name, value); 
									if (typeid(Type) == typeid(uint8))
										return AddUInt8(name, value); 
									if (typeid(Type) == typeid(int16))
										return AddInt16(name, value); 
									if (typeid(Type) == typeid(uint16))
										return AddUInt16(name, value); 
									if (typeid(Type) == typeid(int32))
										return AddInt32(name, value); 
									if (typeid(Type) == typeid(uint32))
										return AddUInt32(name, value); 
									if (typeid(Type) == typeid(int64))
										return AddInt64(name, value); 
									if (typeid(Type) == typeid(uint64))
										return AddUInt64(name, value); 
									if (typeid(Type) == typeid(bool))
										return AddBool(name, value); 
									if (typeid(Type) == typeid(float))
										return AddFloat(name, value); 
									if (typeid(Type) == typeid(double))
										return AddDouble(name, value); 
								};
			
			template <typename Type>
			status_t			Find(const char* name, Type *value) {
									if (typeid(Type) == typeid(int32))
										return FindInt32(name, value); 
								}
			template <typename Type>
			status_t			Find(const char* name, int32 index, Type *value) {
									if (typeid(Type) == typeid(int32))
										return FindInt32(name, index, value); 
								}
		
			// template <typename Type = BAlignment>
			// status_t			Add(const char* name, Type value) { return AddAlignment(name, value); };
			// template <typename Type = BRect>				
			// status_t			Add(const char* name, Type value) { return	AddRect(name, value); }
			// template <typename Type = BPoint>
			// status_t			Add(const char* name, Type value) { return	AddPoint(name, value); }
			// template <typename Type = BPoint>
			// status_t			AddSize(const char* name, Type value) { return	AddPoint(name, value); };
			// status_t			AddString(const char* name, const char* string);
			// status_t			AddString(const char* name,
									// const BString& string);
			// status_t			AddStrings(const char* name,
									// const BStringList& list);
			// status_t			AddInt8(const char* name, int8 value);
			// status_t			AddUInt8(const char* name, uint8 value);
			// status_t			AddInt16(const char* name, int16 value);
			// status_t			AddUInt16(const char* name, uint16 value);
			// status_t			AddInt32(const char* name, int32 value);
			// status_t			AddUInt32(const char* name, uint32 value);
			// status_t			AddInt64(const char* name, int64 value);
			// status_t			AddUInt64(const char* name, uint64 value);
			// status_t			AddBool(const char* name, bool value);
			// status_t			AddFloat(const char* name, float value);
			// status_t			AddDouble(const char* name, double value);
			// status_t			AddColor(const char* name, rgb_color value);
			// status_t			AddPointer(const char* name,
									// const void* pointer);
			// status_t			AddMessenger(const char* name,
									// BMessenger messenger);
			// status_t			AddRef(const char* name, const entry_ref* ref);
			// status_t			AddNodeRef(const char* name,
									// const node_ref* ref);
			// status_t			AddMessage(const char* name,
									// const BMessage* message);
			// status_t			AddFlat(const char* name, BFlattenable* object,
									// int32 count = 1);
			// status_t			AddFlat(const char* name,
									// const BFlattenable* object, int32 count = 1);
		};
	
	template <typename ResultType>
	class TaskResult: public BArchivable {
	public:
		
		TaskResult(const BMessage &archive) 
		{
			archive.PrintToStream();
			void *data = nullptr;
			ssize_t size = 0;
			type_code type;
			
			if (archive.GetInfo(fResultFieldName, &type) == B_OK) {
				status_t error = archive.FindData(fResultFieldName, type, 
													(const void**)&data, &size);
				if (error != B_OK)
					throw runtime_error("Can't unarchive TaskResult instance");
				else
					fResult = *(ResultType*)(data);
			} else {
				throw runtime_error("Can't unarchive TaskResult instance");
			}
			
			if (archive.FindInt32(fResultTaskIdName, &fId) != B_OK) {
					throw runtime_error("Can't unarchive TaskResult instance");
			}
			// if (archive.Find<int32>(fResultTaskIdName, &fId) != B_OK) {
					// throw runtime_error("Can't unarchive TaskResult instance");
			// }
		}
		
		~TaskResult() {}

		ResultType	GetResult() 
		{ 
			// BString log("log: size=");
			// log << TaskExceptionMap.size();
			// log << " ";
			// for(auto &e : TaskExceptionMap) {
				// log << " fid=" << fId;
				// log << " id=" << e.first;
				// log << " ptr=" << (e.second == nullptr ? "false" : "true");
			// }
			// OKAlert("", log, B_INFO_ALERT);
			
			auto ptr = TaskExceptionMap[fId];
			if (ptr)
				rethrow_exception(ptr);
			else
				return fResult; 
		}
		
		virtual status_t Archive(BMessage *archive, bool deep)
		{
			status_t result;
			result = BArchivable::Archive(archive, deep);
			
			type_code type = BMessageType<ResultType>::Get();
			result = archive->AddData(fResultFieldName, type, &fResult, sizeof(fResult));
			result = archive->AddInt32(fResultTaskIdName, fId);
			
			result = archive->AddString("class", "TaskResult");
		   
			return result;
		}
		
		static BArchivable* Instantiate(BMessage* archive)
		{
			if (validate_instantiation(archive, "TaskResult"))
				return new TaskResult(archive);
			else
				return nullptr;
		}

	private:
		friend class	Task<ResultType>;
		const char*		fResultFieldName = "TaskResult::Result";
		const char*		fResultTaskIdName = "TaskResult::ID";
		ResultType		fResult;
		thread_id		fId;
		
						TaskResult() {}
		
	};

	template <typename ResultType>
	class Task {
	public:
		using native_handle_type = thread_id;

		template<typename Name, typename Messenger, typename Function, typename... Args>
		Task(Name name, Messenger messenger, Function&& function, Args&&... args)
		{
			auto *target_function =
				new arguments_wrapper<Function, Args...>(std::forward<Function>(function),
				std::forward<Args>(args)...);
			using lambda_type = std::remove_reference_t<decltype(*target_function)>;

			ThreadData *thread_data = new ThreadData;
			thread_data->target_function = target_function;
			thread_data->messenger = messenger;
			
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
								
								
		
	private:
		friend class TaskResult<ResultType>;
		
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
				auto result = (*lambda)();
				task_result.fResult = result;
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

			ResultType operator()()
			{
				return apply(std::make_index_sequence<sizeof...(Args)>{});
			}
		private:
			template <std::size_t ... indices>
			ResultType apply(std::index_sequence<indices...>)
			{
				return callable(std::move(std::get<indices>(args))...);
			}

		};
	};

}