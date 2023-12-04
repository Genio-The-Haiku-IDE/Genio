/*
 * Copyright 2023, Nexus6 <nexus6.haiku@icloud.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#pragma once

#include "Task.h"

#include <functional>

#include <Handler.h>
#include <Looper.h>
#include <Message.h>
#include <MessageFilter.h>

#include "BeDC.h"

// #define task(name, messenger) "{ LambdaTask task(name, messenger, [&]()"
// #define task(...) { Genio::Task::LambdaTask task(__VA_OPT__(,) __VA_ARGS__, [&]()
//
// #define onexit(result) ,[&](result)
//
// #define finalize );}


namespace Genio::Task {

	namespace Private {

			class TaskHandler : public BHandler {
			public:
				TaskHandler(const char *name, const std::function<void(const TaskResult<void>)> lambda)
					:
					BHandler(name),
					onexit(lambda)
				{
					BeDC("LambdaTask").SendMessage("Handler::Handler()");
				}

				virtual ~TaskHandler()
				{
					BeDC("LambdaTask").SendMessage("Handler::~Handler()");
				}

				void MessageReceived(BMessage *msg)
				{
					BeDC("LambdaTask").SendMessage("TaskHandler:MessageReceived");
					// switch (msg->what) {
						// case TASK_RESULT_MESSAGE: {
							try {
								auto result = TaskResult<void>::Instantiate(msg);
								BeDC("LambdaTask").SendFormat("target found for %s", result->GetTaskName());
								onexit(*result);
							} catch(std::exception &e) {
							}
							// break;
						// }
						// default:
							// break;
					// }
					BHandler::MessageReceived(msg);
				}

			private:
				const std::function<void(const TaskResult<void>)> onexit;
			};

			class MessageFilter : public BMessageFilter
			{
			public:
				MessageFilter(uint32 command)
					:
					BMessageFilter(B_ANY_DELIVERY, B_ANY_SOURCE, command)
				{
					BeDC("LambdaTask").SendMessage("MessageFilter()");
					fMap.clear();
				}

				virtual ~MessageFilter()
				{
					BeDC("LambdaTask").SendMessage("~MessageFilter()");
					for (auto &handler: fMap)
						delete handler.second;
				}

				void AddHandler(const char *taskName, BHandler *handler)
				{
					// fMap["Fetch"] = handler;
					fMap[taskName] = handler;
					if (Looper()->LockLooper()) {
						Looper()->AddHandler(handler);
						Looper()->UnlockLooper();
					}
				}

				virtual	filter_result Filter(BMessage* message, BHandler** target)
				{
					message->PrintToStream();
					BeDC("LambdaTask").SendFormat("Filter()");
					BeDC("LambdaTask").DumpBMessage(message);
					// switch (message->what) {
						// case TASK_RESULT_MESSAGE: {
							auto name = BString(message->GetString("TaskResult::TaskName"));
							*target = fMap[name];
							BeDC("LambdaTask").SendFormat("target found for %s", name.String());
							// break;
						// }
						// default:
						// break;
					// }
					return B_DISPATCH_MESSAGE;
				}

			private:
				typedef std::map<BString, BHandler*> HandlerMap;
				HandlerMap fMap;
			};

	}

	using namespace Private;

	class LambdaTask {
	public:

		LambdaTask(const char* name, const BMessenger& messenger,
			const auto& lambda, const std::function<void(const TaskResult<void>)> onexit)
		{
			InstallTaskHandler(messenger);
			Task<void> task(name, messenger, lambda);
			auto handler = new TaskHandler(name, onexit);
			fFilter->AddHandler(name, handler);
			task.Run();
		}

		~LambdaTask() {}

		void InstallTaskHandler(BMessenger messenger) {
			try {
				BHandler *target = nullptr;
				target = messenger.Target(nullptr);
				if (target != nullptr) {
					if (target->LockLooper()) {
						fFilter = new MessageFilter(TASK_RESULT_MESSAGE);
						target->AddFilter(fFilter);
						target->UnlockLooper();
					}
				}
			} catch(...) {}
		};

		void RemoveTaskHandler(BMessenger messenger) {
			try {
				BHandler *target = nullptr;
				target = messenger.Target(nullptr);
				if (target != nullptr) {
					if (target->LockLooper()) {
						target->RemoveFilter(fFilter);
						target->UnlockLooper();
					}
				}
				delete fFilter;
			} catch(...) {}
		}

	private:
		BMessenger fMessenger;
		MessageFilter *fFilter;
	};
}