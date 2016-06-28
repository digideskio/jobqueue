/* Single threaded scheduled job queue.
 *
 * Goals in this pass, switch to array of structs instead of array of pointers
 * to structs. Insert in sorted order. Use a little extra storage to save CPU.
 */

#include <iostream>
#include <functional>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <cmath>


typedef unsigned long long uint64;
typedef std::function<void()> Callback;

struct Job
{
  uint64 whenMs;
  Callback callback;
};

static const size_t jobSize = sizeof(Job);

struct JobQueue
{
  // Double becaucse input time is a float. Doubles will hold integers until
  // they exceed 53 bits per IEEE 754. That means we can hold about 285420 years
  // worth of milliseconds.
  double currentTimeMs;
  uint64 nextJobTime;
  uint64 lastJobTime;
  size_t jobCount;
  size_t maxJobs;
  Job *jobs;
};

void* MallocZero(size_t size);

JobQueue JobQueue_Init(size_t maxJobs);
bool JobQueue_AddJob(JobQueue& jobQueue, uint64 offsetMs, Callback callback);
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
  memset(&jq, 0, sizeof(JobQueue));

  jq.maxJobs = maxJobs;
  // Allocate the buffer for jobs and 0 it out so we get nullptr if we access an
  // element that is in range but doens't exist.
  jq.jobs = static_cast<Job*>(MallocZero(jobMemorySize));

  return jq;
}

void JobQueue_Shutdown(JobQueue& jobQueue)
{
  free(jobQueue.jobs);
  return;
}

// offsetMs is a minimum, not a maximum. We are called inconsistently so we
// can't assure that we'll call the callback exactly at time.
bool JobQueue_AddJob(JobQueue& jobQueue, uint64 offsetMs, Callback callback)
{
  bool added = false;
  if (jobQueue.jobCount < jobQueue.maxJobs) {
    std::cout << "adding new job" << std::endl;
    // Use std::ceil instead of std::floor so we can assure minimum time because
    // we can't assure maximum, assuring neither is worse.
    Job job = Job{
      .whenMs = static_cast<uint64>(std::ceil(jobQueue.currentTimeMs)) + offsetMs,
      .callback = callback = callback
    };

    if (job.whenMs >= jobQueue.lastJobTime) {
      std::cout << "new job is higher than exisiting jobs inserting into position: " << jobQueue.jobCount << std::endl;
      jobQueue.jobs[jobQueue.jobCount] = job;
      jobQueue.lastJobTime = job.whenMs;
    } else if (job.whenMs < jobQueue.nextJobTime) {
      std::cout << "new job is lower than exisiting jobs" << std::endl;
      std::memmove(&jobQueue.jobs[1], &jobQueue.jobs[0], jobQueue.jobCount * jobSize);
      jobQueue.jobs[0] = job;
      jobQueue.nextJobTime = job.whenMs;
    } else {
      for (size_t i = 0; i < jobQueue.jobCount; i++) {
        if (jobQueue.jobs[i].whenMs > job.whenMs) {
          std::memmove(&jobQueue.jobs[i + 1],
                       &jobQueue.jobs[i],
                       (i - jobQueue.jobCount) * jobSize);
          jobQueue.jobs[i] = job;
          break;
        }
      }
    }
    if (jobQueue.jobCount == 0) {
      jobQueue.nextJobTime = job.whenMs;
      jobQueue.lastJobTime = job.whenMs;
    }
    jobQueue.jobCount++;
    added = true;
  }
  return added;
}

size_t JobQueue_CountJobs(const JobQueue& jobQueue)
{
  return jobQueue.jobCount;
}

// The float mirrors the system I'll be integrating with, despite it being less
// convenient. Theoretically inserts are safe unless they insert with 0 time. If
// we force all inserts to be at least 1ms after jobQueue.currentTimeMs newly
// inserted jobs will have to be after the jobs that we are assured we are only
// deleting old jobs. This does mean that while executing callbacks added jobs
// will see the old size of the array.
void JobQueue_Update(JobQueue& jobQueue, float _timeSinceLastUpdateMs)
{
  jobQueue.currentTimeMs += _timeSinceLastUpdateMs;

  if (jobQueue.currentTimeMs >= jobQueue.nextJobTime) {
    size_t remaining = jobQueue.jobCount;
    for (size_t i = 0; i < jobQueue.jobCount; i++) {
      Job job = jobQueue.jobs[i];

      if (job.whenMs < jobQueue.currentTimeMs) {
        std::cout << "running job: " << i << std::endl;
        remaining--;
        job.callback();
      } else {
        jobQueue.nextJobTime = job.whenMs;
        break;
      }
    }

    if (remaining != jobQueue.jobCount) {
      if (remaining != 0) {
        // Sorted insert into the array means we know the rest are in the
        // future. Move the rest to the start of the array.
        std::memmove(&jobQueue.jobs[jobQueue.jobCount - remaining],
                     &jobQueue.jobs[0],
                     remaining * jobSize);
        // Mostly for debugging, make it easier when inspecting the array to see
        // what is left in the array.
        std::memset(&jobQueue.jobs[remaining], 0, (jobQueue.maxJobs - remaining) * jobSize);
      } else {
        // no more jobs remaining, zero stuff out and keep going.
        std::memset(&jobQueue.jobs[0], 0, jobQueue.maxJobs * jobSize);
        jobQueue.nextJobTime = 0;
        jobQueue.lastJobTime = 0;
      }
      jobQueue.jobCount = remaining;
    }
  }
}
