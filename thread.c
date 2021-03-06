/*  =====================================================================
	In this lab, you will implement several versions of a code which
	computes a histogram of a set of random numbers.  While seemingly
	simple, there are many possible optimizations.  Also, this is a key
	kernel in a variety of algorithms (eg. radix sort).

	A skeleton of the lab is provided which already spawns NTHREADS threads
	for you, and calls the various kernels which you are to implement.
	For each kernel (histo_1,histo_2, etc...), we describe an idea
	of how to parallelize that you should follow, along with some hint.
	
    =====================================================================
*/

#include "thread.h"

info_t info = {
  "Avery Einhorn",
  "405115287",
};
/*  =====================================================================
	YOUR CODE GOES HERE:
	
	Add corresponding locks/semaphores and any other global variables here

	Sample declarations:
========================================================================*/	
	sem_t mutex; //don't forget to initialize in main

        pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
        pthread_barrier_t barrier; //don't forget to initialize in main
        pthread_mutex_t locks[BUCKET_SIZE]; //don't forget to initialize in main

/*=====================================================================
	END YOUR CODE HERE
  =====================================================================
*/

int main() {
/*=====================================================================
	YOUR CODE GOES HERE:
  Initialize your locks here
  =====================================================================
  */
	sem_init(&mutex,0,1); 
	 
  /*=====================================================================
	END YOUR CODE HERE
  DO NOT TOUCH THE REST OF MAIN
    =====================================================================
  */
 	bool isComplete = check_info(info);
	if(isComplete) run_threads();

	return 0;
}


/*  =====================================================================
	Kernel to Parallelize
    =====================================================================
    =====================================================================

	This is the code we would like to parallelize, it computes
	a histogram of our randomly generated dataset.  This particular
	histogram counts the number of data items with a given remainder
	when divided by BUCKET_SIZE; we use the modulo (%) operator 
	for that.

	Note that even though this function is called with multiple threads,
	only one thread performs the computation.  It is sequential.
	
    =====================================================================
*/
void *histo_0(void *vargp){
 	int thread_id = (long int)vargp;
	if(thread_id==0) { //only run on thread 0
		int j;
		for (j=0; j<DATA_SIZE; j++){
			global_histogram[data[j]%BUCKET_SIZE]++;
		}
	}       
}


/*  =====================================================================
	BEGIN TASK 1 (mandatory)
    =====================================================================
    =====================================================================

	The given code divides the data into blocks, one for each
	thread.  However, there is a race! (where?)

	Try to add a global lock and modify this thread routine.

	HINT: You might want to apply the lock each time the associated
	counter changes

    =====================================================================
*/

void *histo_1(void *vargp){
	int ind = (long int)vargp;
	ind = ind*STEP;
	int j;
	for (j=ind; j<ind+STEP; j++){
		pthread_mutex_lock(&lock);
		global_histogram[data[j]%BUCKET_SIZE]++;
		pthread_mutex_unlock(&lock);
	}
	return NULL;
}

/*  =====================================================================
	QUESTION 1:
		After adding the global lock, do you observe any 
		performance changes (i.e. correctness and time) compared
		to histo_0 and histo_1 without the lock? If so, 
		think about the reasons behind the changes.
	
    =====================================================================   
	Correctness is just as correct as hist_0, but the time is much much longer because the program has to constantly be going to lock and unlock the mutex which is costly in terms of time 
   =====================================================================
	END TASK 1
    =====================================================================
*/



/*  =====================================================================
	BEGIN TASK 2 (mandatory)
    =====================================================================
    =====================================================================

	One problem with the previous implementation is that many threads
	access the lock at the same time.  One possible solution is to
	have one lock per histogram bucket.

	Try to add a bucket-specific lock and modify this thread routine.

	HINT: You might want to use pthread_mutex_init to initialize each
	bucket-specific lock before using threads.

    =====================================================================
*/

void *histo_2(void *vargp){
	int ind = (long int)vargp;
	ind = ind*STEP;
	int j;
	for (j=ind; j<ind+STEP; j++){
		pthread_mutex_lock(&locks[data[j]%BUCKET_SIZE]);	
		global_histogram[data[j]%BUCKET_SIZE]++;
		pthread_mutex_unlock(&locks[data[j]%BUCKET_SIZE]);
	}
	return NULL;
}

/*  =====================================================================
	QUESTION 2:
		After adding the bucket-specific lock, do you observe any 
		performance changes (i.e. correctness and time) compared 
		to histo_1 (with global lock)? If so, think about
    the reasons behind the changes.

    =====================================================================
	Correctness is still good, because we protected everything, and time improves because the lock is bucket-specific.
    =====================================================================
	END TASK 2
    =====================================================================
*/



/*  =====================================================================
	BEGIN TASK 3 (mandatory)
    =====================================================================
    =====================================================================
    	
	Locks enable us to put anything within a critical section, but
	notice that here we are performing only a single increment within
	the critical section.  The X86 ISA (and others) have a special
	instruction to perform this operation -- this might be faster.

	Try to use atomic increment and modify this thread routine.
	
	HINT: You might want to check the Atomic Builtin function called 
	__sync_fetch_and_add

    =====================================================================
*/

void *histo_3(void *vargp){
	int ind = (long int)vargp;
	ind = ind*STEP;
	int j;
	for (j=ind; j<ind+STEP; j++){
		__sync_fetch_and_add(&global_histogram[data[j]%BUCKET_SIZE],1);
	}
}

/*  =====================================================================
	QUESTION 3:
		After using atomic increment, do you observe any 
		performance changes (i.e. correctness and time) compared 
		to histo_1 (with global lock)? If so, think about
		the reasons behind the changes.

    =====================================================================
	Correctness is still good :) but time is EVEN BETTER than the bucket-specific lock, because we are using atomic instructions which are much less costly than locking, changing, unlocking the variable manually. 
   =====================================================================
	END TASK 3
    =====================================================================
*/



/*  =====================================================================
	BEGIN TASK 4 (mandatory)
    =====================================================================
    =====================================================================
	
  Approach 1: 
  	It seems that in order to get speedup, we need to eliminate
          fine-grain synchronization.  One simple idea is for each thread
          to perform the histogram operation in its own local array, 
          and only synchronize to accumulate the final sums.
  
  	Try to implement the local histogram-based method.  
  
  	HINT: You might want to use a pthread_barrier to synchronize the
  	threads before you perform the reduction on one thread.
  	
  	ATTENTION: this task is the most difficult one
   
   Approach 2:
     Your own optimizations

    =====================================================================
*/

void *histo_4(void *vargp){
	int ind = (long int)vargp;
	ind = ind*STEP;
	int j;
	int sums[BUCKET_SIZE]={0};
	for (j=ind; j<ind+STEP; j++){
		sums[data[j]%BUCKET_SIZE]++;
	}
	for (j=0; j<BUCKET_SIZE; j++){
		__sync_fetch_and_add(&global_histogram[j],sums[j]);
	}
}

/*  =====================================================================
	QUESTION 4:
		After implementing local histograms, do you observe any 
		performance changes (i.e. correctness and time) compared 
		to implementation of a global histogram? If so, think
		about the reasons behind the changes.
   
	Time went way down, because there was no locking and unlocking, and no fetching global arrays (except via the atomic function, which is pretty useful)

   QUESTION 5:
		For these 4 ways of parallelization, which one do you
		think is the best and why? Try to reason about it for yourself.
	I think that the hist_4 result, while objectively the best performance time, is probably not the best way to parralelize in general. I would probably prefer to use the method in hist_2, because it protects each of our values independantly, and is not dependant on atomic functions. We won't always have atomic functions to do what we need to a value, so relying on them as a method of syncing can be dangerous in the long run. 

    =====================================================================
    =====================================================================
	END TASK 4
    =====================================================================
*/

