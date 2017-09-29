#include "Core/TaskProcessors/AsyncChainProcessor.h"
#include "Core/GuiTasks/AsyncChainTask.h"

AsyncChainProcessor::AsyncChainProcessor(QObject* parent)
    : QObject(parent)
{
}

void AsyncChainProcessor::OnFinished()
{
    tasks.pop_front();

    running = false;
    StartNextTask();
}

void AsyncChainProcessor::AddTask(std::unique_ptr<BaseTask>&& task, Notifier notifier)
{
    Q_ASSERT(task->GetTaskType() == BaseTask::ASYNC_CHAIN);
    AsyncChainTask* chainTask = static_cast<AsyncChainTask*>(task.get());
    chainTask->SetNotifier(notifier);
    connect(chainTask, &AsyncChainTask::Finished, this, &AsyncChainProcessor::OnFinished);

    tasks.emplace_back(std::move(task), notifier);

    StartNextTask();
}

void AsyncChainProcessor::StartNextTask()
{
    if (running)
    {
        return;
    }
    if (tasks.empty() == false)
    {
        running = true;

        TaskParams& params = tasks.front();
        params.notifier.NotifyStarted(params.task.get());
        params.notifier.NotifyProgress(params.task.get(), 0);
        params.task->Run();
    }
}

void AsyncChainProcessor::Terminate()
{
    //do nothing for now
}

std::size_t AsyncChainProcessor::GetTasksCount() const
{
    return tasks.size();
}

AsyncChainProcessor::TaskParams::TaskParams(std::unique_ptr<BaseTask>&& task_, Notifier notifier_)
    : task(static_cast<AsyncChainTask*>(task_.release()))
    , notifier(notifier_)
{
}

AsyncChainProcessor::TaskParams::~TaskParams()
{
    notifier.NotifyFinished(task.get());
}
