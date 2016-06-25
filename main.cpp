#include <iostream>
#include <functional>
#include <stdlib.h>
#include <stdio.h>
#include <cmath>


typedef unsigned long long uint64;
typedef std::function<void()> Callback;

struct Job
{
  uint64 whenMs;
  Callback callback;
};

struct JobQueue
{
  double currentTimeMs;
  size_t maxJobs;
  Job **jobs;
};

void* MallocZero(size_t size);

JobQueue JobQueue_Init(size_t maxJobs);
bool JobQueue_AddJob(JobQueue& jobQueue, uint64 offsetMs, Callback callback);
size_t JobQueue_CountJobs(const JobQueue& jobQueue);
void JobQueue_Update(JobQueue& jobQueue, float _timeSinceLastUpdateMs);

int main ()
{
  JobQueue jobQueue = JobQueue_Init(10);

  bool added = JobQueue_AddJob(jobQueue, 10, []() {
      std::cout << "testing!" << std::endl;
    });

  std::cout << "job added: " << added << std::endl;
  std::cout << "job count: " << JobQueue_CountJobs(jobQueue) << std::endl;
  std::cout << "adding 11.0 to time" << std::endl;
  JobQueue_Update(jobQueue, 11.0);
  std::cout << "job count: " << JobQueue_CountJobs(jobQueue) << std::endl;

  return 0;
}

// Safe malloc! Safe as in exits with an error if failed should probably use
// execinfo and backtrace and stuff... but this is a prototype for learning.
// Also it seems to log a bit from the stdlib if malloc fails, so that's cool.
void* MallocZero(size_t size)
{
  void* memory = malloc(size);
  if (memory == nullptr) {
    std::cerr << "malloc failed, exiting" << std::endl;
    exit(EXIT_FAILURE);
  }
  memset(memory, 0, size);
  return memory;
}

JobQueue JobQueue_Init(size_t maxJobs)
{
  size_t jobMemorySize = maxJobs * sizeof(Job*);
  JobQueue jq;
  // Default everything to 0/nullptr. This way if I add a member it gets initialized.
  memset(&jq, 0, sizeof(JobQueue));

  jq.maxJobs = maxJobs;
  // Allocate the buffer for jobs and 0 it out so we get nullptr if we access an
  // element that is in range but doens't exist.
  jq.jobs = static_cast<Job**>(MallocZero(jobMemorySize));

  return jq;
}

bool JobQueue_AddJob(JobQueue& jobQueue, uint64 offsetMs, Callback callback)
{
  Job *job = static_cast<Job*>(MallocZero(sizeof(Job)));

  job->whenMs = static_cast<uint64>(std::floor(jobQueue.currentTimeMs)) + offsetMs;
  job->callback = callback;

  bool added = false;
  for (size_t i = 0; i < jobQueue.maxJobs; i++) {
    if (jobQueue.jobs[i] == nullptr) {
      jobQueue.jobs[i] = job;
      added = true;
      break;
    }
  }

  return added;
}

size_t JobQueue_CountJobs(const JobQueue& jobQueue)
{
  size_t count = 0;
  for (size_t i = 0; i < jobQueue.maxJobs; i++) {
    if (jobQueue.jobs[count] != nullptr) {
      count++;
    }
  }

  return count;
}

// The float mirrors the system I'll be integrating with, despite it being less convenient.
void JobQueue_Update(JobQueue& jobQueue, float _timeSinceLastUpdateMs)
{
  jobQueue.currentTimeMs += _timeSinceLastUpdateMs;

  for (size_t i = 0; i < jobQueue.maxJobs; i++) {
    Job *job = jobQueue.jobs[i];
    if (job == nullptr) {
      continue;
    }

    if (job->whenMs < jobQueue.currentTimeMs) {
      // free up the space in the queue before calling in case we add more to
      // the queue in the callback. This means we could have n + 1 jobs in
      // existance but only n jobs in the queue.
      jobQueue.jobs[i] = nullptr;
      job->callback();
      free(job);
    }
  }
}
