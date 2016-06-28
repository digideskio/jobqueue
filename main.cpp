/* Single threaded scheduled job queue.
 *
 * This pass I'm going to try an array swapping technique.
 */

#include <iostream>
#include <functional>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <cmath>


typedef std::function<void()> Callback;

struct Job
{
  double whenMs;
  Callback callback;
};

static const size_t jobSize = sizeof(Job);

struct JobQueue
{
  // Double becaucse input time is a float. Doubles will hold integers until
  // they exceed 53 bits per IEEE 754. That means we can hold about 285420 years
  // worth of milliseconds.
  double currentTimeMs;
  size_t currentJobCount;
  size_t nextJobCount;
  size_t maxJobs;
  Job *currentJobs;
  Job *nextJobs;
};

void* MallocZero(size_t size);

JobQueue JobQueue_Init(size_t maxJobs);
bool JobQueue_AddJob(JobQueue& jobQueue, double offsetMs, Callback callback);
size_t JobQueue_CountJobs(const JobQueue& jobQueue);
void JobQueue_Update(JobQueue& jobQueue, float _timeSinceLastUpdateMs);
void JobQueue_Shutdown(JobQueue& jobQueue);

int main ()
{
  JobQueue jobQueue = JobQueue_Init(10);

  bool added = false;
  added = JobQueue_AddJob(jobQueue, 10, []()
                  {
                    std::cout << "added first, runs second" << std::endl;
                  });
  std::cout << "job added: " << (added ? "true" : "false") << std::endl;
  added = JobQueue_AddJob(jobQueue, 5, []()
                  {
                    std::cout << "added second, runs first" << std::endl;
                  });
  std::cout << "job added: " << (added ? "true" : "false") << std::endl;

  std::cout << "job count: " << JobQueue_CountJobs(jobQueue) << std::endl;
  std::cout << "adding 5.0 to time" << std::endl;
  JobQueue_Update(jobQueue, 5.0);
  std::cout << "adding 6.0 to time" << std::endl;
  JobQueue_Update(jobQueue, 6.0);
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
  size_t jobMemorySize = maxJobs * jobSize;
  JobQueue jq;
  // Default everything to 0/nullptr. This way if I add a member it gets initialized.
  memset(&jq, 0, sizeof(jq));

  jq.maxJobs = maxJobs;
  jq.currentJobs = static_cast<Job*>(MallocZero(jobMemorySize));
  jq.nextJobs = static_cast<Job*>(MallocZero(jobMemorySize));

  return jq;
}

void JobQueue_Shutdown(JobQueue& jobQueue)
{
  free(jobQueue.currentJobs);
  free(jobQueue.nextJobs);
  return;
}

// offsetMs is a minimum, not a maximum. We are called inconsistently so we
// can't assure that we'll call the callback exactly at time.
bool JobQueue_AddJob(JobQueue& jobQueue, double offsetMs, Callback callback)
{
  bool added = false;
  if (jobQueue.nextJobCount < jobQueue.maxJobs) {
    std::cout << "adding new job" << std::endl;
    Job job = Job{
      .whenMs = jobQueue.currentTimeMs + offsetMs,
      .callback = callback = callback
    };

    jobQueue.nextJobs[jobQueue.nextJobCount++] = job;

    added = true;
  }
  return added;
}

size_t JobQueue_CountJobs(const JobQueue& jobQueue)
{
  return jobQueue.nextJobCount;
}

void JobQueue_Update(JobQueue& jobQueue, float _timeSinceLastUpdateMs)
{
  jobQueue.currentTimeMs += _timeSinceLastUpdateMs;

  {
    Job* tmp = jobQueue.currentJobs;
    jobQueue.currentJobs = jobQueue.nextJobs;
    jobQueue.currentJobCount = jobQueue.nextJobCount;
    jobQueue.nextJobs = tmp;
    jobQueue.nextJobCount = 0;
  }

  for (size_t i = 0; i < jobQueue.currentJobCount; i++) {
    Job job = jobQueue.currentJobs[i];

    if (job.whenMs < jobQueue.currentTimeMs) {
      std::cout << "running job: " << i << std::endl;
      job.callback();
    } else {
      // For safty but it means if jobs add more jobs, then we could run out of
      // space and lose one... not good.
      if (jobQueue.nextJobCount < jobQueue.maxJobs) {
        jobQueue.nextJobs[jobQueue.nextJobCount++] = job;
      } else {
        std::cerr << "lots job because nextJobs got full while evaluating the current jobs";
      }
    }
  }
  jobQueue.currentJobCount = 0;
}
