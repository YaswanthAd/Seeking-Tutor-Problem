// *
// ------------------------------------------------------------------------------------------------------------------------------

// CS 5348 Operating Systems (Project 3 - Seeking Tutor Problem)
// Instructor: Prof. Sridhar Alagar

// ------------------------------------------------------------------------------------------------------------------------------

// Team member contributions (though all of us worked together for all the modules, main contributions have been listed below): 

// 1) Yaswanth Adari
//    NET ID: YXA220006

// 2) Sai Pavan Goud Addla
//    NET ID: SXA210294

// ------------------------------------------------------------------------------------------------------------------------------

// How to execute the code? (csgrads1 server)

// 1. Compilation : gcc csmc.c –o csmc -Wall -Werror -pthreads -std=gnu99
// 2. Running the code : ./csmc

// ------------------------------------------------------------------------------------------------------------------------------

//Including Libraries
#include <pthread.h>
#include <unistd.h>
#include <stdlib.h>
#include <semaphore.h>
#include <stdio.h>
#include <time.h>
#include <string.h>
#include <assert.h>

// Defining macros used in the below code
#define maxarg 5            //  Maximum number of arguments that can be passed
#define maxstu 2100        //  Maximum number of students compatiable with this code, without throwing error
#define maxtutors 2100    //   We are declaring Maxtutors same as student inorder to initalise the student array
#define maxchairs 20     //    Maximum number of chairs that can passed to the arguments  
#define minchairs 1     //     Minimum number of chairs that can be passed to the arguments
#define minstu 1       //      Minimum number of chairs that can be passed to the arguments
#define mintutors 1   //       Minimum number of tutors that can be passed to the arguments
#define minhelps 1   //        Minimum number of helps that can be passed to the arguments
#define maxqueue 2100           // initalising max queue as same as student class
#define maxcoordinator 1       //  Maximum number of coordinators used in this code/passed via arguments
#define intialiser 0          //   Declaring intialiser as 0 inorder to initalise the threads


//*****************************STRUCT DECLARATIONS*****************************//

//Student Struct Data
struct StudentMetaData
{
    int studentID[maxstu]; // This variable used to store array of integers maintained to know the ID's of the students.
    //sem_t tutorAvailableSemaphore;
};

// struct CoordinatorMetaData
// {
//     int* studentID;
// }

//Tutor Struct Data
struct TutorMetaData
{
    int tutorID[maxstu]; // This Variable used to store array of integers maintained to know the ID's of the tutors.
};

//**************************OBJECT DECLARATIONS *******************//

struct StudentMetaData* studentObject;  // Stores the student Object which is used to reference student ID
struct TutorMetaData* tutorObject;     //  Stores the tutor Object which is used to reference tutor ID

// ************************THREAD CREATION FUNCTIONS*******************//
void *studentThread(void *student);
void *TutorThread(void *tutor);
void *CoordinatorThread(void *coordinator);

// ********************* OUTPUT VARIABLES DECLARATIONS *********************** //
int RequestsDone = 0;
int totalRequests = 0;
int CompleteSessions = 0;
int SessionTutoring = 0;
int ChairsOccupied = 0;

// ******************** INPUT VARIABLE DECLARATIONS ************* //
int numberOfstudents;
int numberOftutors;
int helpsPerPerson;
int totalNumberOfChairs;
int numberofcoordinators = 1;

// ******************* DECLARING ARRAYS ************* //
int QueueForNewStudents[maxstu];         //  Array in order to store the newly arrived students list and the size is same as maximum number of students
int QueueForTutors[maxstu];             //   Array in order to store the tutors for picking up the students
int PriorityQueue[maxstu][2];          //    2D array used to store the students according to their priority
int QueueForStudentsPriority[maxstu]; //     Array to store the students picked to tutored first
int coordinatorID[maxcoordinator];   //      Array Declaration of coordinator

// ****************SEMAPHORES DECLARATION*************** //
sem_t student_waiting_for_coordinator;  //Semaphore used to wake up the coordinator when a student comes
sem_t coordinator_waiting_for_tutor;   // Semaphore used to wake up tutor when a student is added into queue by the coordinator

// *****************LOCKS DECLARATION***************** //
pthread_mutex_t AvailableSeatlock;           // Used to lock the available chairs
pthread_mutex_t QueueLockByTutor;           //  Used to lock the queue from which tutors select the students
pthread_mutex_t TutorConfirmation;         //   Used to lock the students entering the queue/array
pthread_mutex_t SessionTutoringLock;      //    Used to lock the shared variable 


// ************ FUNCTION TO GENERATE RANDOM VALUES FOR SLEEP ************* //
// We are passing the data to the random function which returns the appropriate number based on thread
// Returns 2000 to student thread 
// Returns 200 to tutor thread

int randomfunction(int data)
{
    int temporary = 0;
    for(temporary = 0; temporary < totalNumberOfChairs; temporary++)
    {
        if(data == 0)
        {
            return 2000;
            break;
        }
        else
        {
            return 200;
            break;
        }
    }
   return 0;
}

// ************ FUNCTION TO CHECK WHETHER THE PASSED ARGUMENTS LIES IN THE APPROPRIATE RANGE ************ //
// Returns 0 if the numberofstudents passed are greather than the max limit
// Returns 1 if the numberofstudents passed are in the range
int CasesWorking()
{
    if(numberOfstudents > maxstu)
    {
        return 0;
    }
    else
    {
        return 1;
    }
}

// ****************** FUNCTION TO INITALISE THE ARRAYS ************* //
// Here before initalising, we are checking whether the number of students less than the max size

void ArrayIntialiser()
{
    int itr = 0;
    while(itr < numberOfstudents)
    {
        if(CasesWorking())
        {
            QueueForStudentsPriority[itr]=0;
            QueueForNewStudents[itr] = -1;
            PriorityQueue[itr][0] = -1;
            PriorityQueue[itr][1] = -1;
            QueueForTutors[itr] = -1;
            itr++;
        }
        else
        {
            itr++;
        }
    }
}

// *************** THREADS DECLARATIONS ***************** //

// student thread
void *studentThread(void *student)

{
    //initally we are converting the student parameter/argument from void* to int
    int studentrecord = *(int*) student;
    
    while(CasesWorking())
    {
        //declaring a variable here to test the semaphores whether they successfully returned via assert
        int test;
        // checking whether the number of helps a particular person/students exceeds the given limit of helps per person
        if(QueueForStudentsPriority[studentrecord-1] < helpsPerPerson) 
        {
            //Declaring a int to find the random amount of time for the student to sleep by passing it to a funciton as an argument
            int random = randomfunction(intialiser);

            //student coming into the student sleeping for random amount of time based on the input(2000 ms)
            usleep(rand() % random);           

            //Locking the chair before entering the critical section to find out if the chairs occupied less than the total number of the chairs argument
            pthread_mutex_lock(&AvailableSeatlock);

            //The critical section where we are checking if the chairs occupied less/greather than the argument(number of chairs provided) by the user
            if(ChairsOccupied < totalNumberOfChairs)
            {
                //increasing the number of requests by 1 as we proceed down the code
                totalRequests  = totalRequests + 1;
                
                //decreasing the chairs occupied by 1, as each student takes the seat
                ChairsOccupied = ChairsOccupied + 1;

                //updating the array with the total number of requests for each student
                QueueForNewStudents[studentrecord-1] = totalRequests;
                
                printf("S: Student %d takes a seat. Empty chairs = <%d of empty chairs after student %d took a seat>.\n",studentrecord,totalNumberOfChairs-ChairsOccupied, studentrecord);

                //unlocking the lock before leaving the critical section
                pthread_mutex_unlock(&AvailableSeatlock);
                
                //Waking up the coordinator to the student into the queue
                test = sem_post(&student_waiting_for_coordinator);

                //asserting whether the test returning the zero or not
                assert(test == 0);
                
                //spinning up the student till a tutor is assigned
                while(QueueForTutors[studentrecord-1] == -1);

                printf("S: Student %d received help from Tutor %d.\n",studentrecord,QueueForTutors[studentrecord-1]-numberOfstudents);
                
                //locking before intialsing the tutor queue(shared variable)
                pthread_mutex_lock(&TutorConfirmation);
                //intialsing the tutor queue with -1 
                QueueForTutors[studentrecord-1] = -1;

                //unlocking before leaving the critical section
                pthread_mutex_unlock(&TutorConfirmation);
                
                //locking the seat before entering the critical section
                pthread_mutex_lock(&AvailableSeatlock);
                //increment the count by 1 for the students entering the prority queue
                QueueForStudentsPriority[studentrecord-1] = QueueForStudentsPriority[studentrecord-1] + 1 ;
                //unlocking the seat before leaving the critical section
                pthread_mutex_unlock(&AvailableSeatlock);   

            }
            else
            {
                printf("S: Student %d found no empty chair. Will try again later.\n",studentrecord);
                printf("The total number of requests till now: %d\n", totalRequests);

                //unlocking the seat before exiting from the loop
                pthread_mutex_unlock(&AvailableSeatlock);
                continue;
            }        

        }
        
        else
        {
            //loop comes here when the number of seats are occupied/full
            //locking the critical section 
            pthread_mutex_lock(&AvailableSeatlock);
            // as we are incrementing the shared variable we are locking the critical section
            RequestsDone = RequestsDone + 1;
            //releasing the lock before leaving the critical section
            pthread_mutex_unlock(&AvailableSeatlock);

            //informing the coordinator to terminate the student
            test = sem_post(&student_waiting_for_coordinator);
            //checking if the assert passes or fails using the test variable
            assert(test == 0);
    
            pthread_exit(NULL);
        }
    }
    //printf("The student term terminated\n");
    pthread_exit(NULL);
}


//tutor thread
void *TutorThread(void *tutor)
{
    //initally we are converting the student parameter/argument from void* to int
    int tutrID = *(int*) tutor;

    int TimeTakenByTutor; // declaring a variable to find the time taken by the tutor
    int test;             // declaring a variable to check whether the semaphores passed or not 
    int StudentTime;      // declaring a variable to find the time taken by the student
    int studentrecord;    // object to refer the studentID
    
    //running a loop to find out whether the cases are working or not
    while(CasesWorking())
    {
        // checking whether the requests done are less than the number of students
        if(RequestsDone < numberOfstudents)
        {
            //intialsing the timetaken by tutor as the helps per person -1 
            TimeTakenByTutor = helpsPerPerson - 1;
            int tot = numberOfstudents * helpsPerPerson;
            
            //intialising the student time
            StudentTime = tot + 1;
            studentrecord = -1;
            
            //Waiting for the coordinator to add student from the queue 
            test = sem_wait(&coordinator_waiting_for_tutor);

            //checking whether the semaphore passed or not using assert
            assert(test == 0);
            
            //locking the chair after student occupies the available chair
            pthread_mutex_lock(&AvailableSeatlock);
            
            //declaring a loop to check which student to pick from the priority queue
            int temp;
            for(temp=0;temp < numberOfstudents;temp++)
            {
                if(PriorityQueue[temp][0] > -1 && PriorityQueue[temp][0] <= TimeTakenByTutor)
                {
                    if((PriorityQueue[temp][0] <= TimeTakenByTutor) && (PriorityQueue[temp][1]<StudentTime))
                    {
                        TimeTakenByTutor=PriorityQueue[temp][0];
                        StudentTime = PriorityQueue[temp][1];
                        studentrecord = studentObject->studentID[temp];
                    }
                }
            }
            
            //checking whether the studentrecord if it is intialised or not, if yes we are unlocking and exiting from the thread
            if(studentrecord < 0) 
            {
                //unlocking before leaving the critical section
                pthread_mutex_unlock(&AvailableSeatlock);
                continue;
            }
            
            //after student gets tutored we are emptying the priority queue and intialsing its place to -1
            PriorityQueue[studentrecord-1][0]= -1;
            PriorityQueue[studentrecord-1][1]= -1;
            
            //locking the variable before entering the critical section
            pthread_mutex_lock(&SessionTutoringLock);
            SessionTutoring = SessionTutoring + 1;

            //releasing the lock before leaving the critical section
            pthread_mutex_unlock(&SessionTutoringLock);

            //As student enters into the tutoring zone, we are updating the chair count
            ChairsOccupied = ChairsOccupied - 1;
            //releasing the chair lock 
            pthread_mutex_unlock(&AvailableSeatlock);

            //calling the random function as students get tutored by the tutor
            int random2 = randomfunction(mintutors);   
            usleep(rand() % random2);
            
            //locking the chair while students get tutored
            pthread_mutex_lock(&AvailableSeatlock);
            
            //updating the completed session as student completes the tutoring
            CompleteSessions = CompleteSessions + 1;
            
            //updating count and declaring the lock
            pthread_mutex_lock(&SessionTutoringLock);
            SessionTutoring = SessionTutoring - 1;
            pthread_mutex_unlock(&SessionTutoringLock);
            
            printf("T: Student %d tutored by Tutor %d. Students tutored now = <%d students receiving help now>. Total sessions tutored = <%d total number of tutoring sessions completed so far by all the tutors>\n",studentrecord,tutrID-numberOfstudents,SessionTutoring,CompleteSessions);

            //unlocking the chair before student leaves after tutoring is complete
            pthread_mutex_unlock(&AvailableSeatlock);

            // getting the information of which tutor, tutored student using tutorID
            pthread_mutex_lock(&TutorConfirmation);

            // updating the tutor queue 
            QueueForTutors[studentrecord-1] = tutrID;
            pthread_mutex_unlock(&TutorConfirmation); 

            
        }
        
        else
        { 
            //if requests done are more than the students, we are exiting from the thread
            pthread_exit(NULL);
           
        }     
    }
    //printf("The tutor thread terminated\n");
    pthread_exit(NULL);
}

//coordinator thread
void *CoordinatorThread(void *coordinator)
{
    //initally we are converting the student parameter/argument from void* to int
    int coodID = * (int*)coordinator;

    // declaring a variable test to check whether the semaphores asserted or not
    int test;
    //running the loop infinite times until the condition is not satisfied
    while(CasesWorking())
    {
        // checking whether the requests done are greather than the number of students occupied
        if(RequestsDone >= numberOfstudents)
        {
            
            int temp = coodID - 1;
            while(temp < numberOftutors)
            {
                //waiking up the tutor by pushing student into the queue
	            test = sem_post(&coordinator_waiting_for_tutor);
                assert(test == 0);
                temp++;
            }
                     
            pthread_exit(NULL);
        }

        else
        {
            // waiting for the student to occupy on the available chairs
            test = sem_wait(&student_waiting_for_coordinator);

            //checking whether the semaphore returned zero or not
            assert(test == 0);
            
            //locking the seat as the student occupies on the chairs
            pthread_mutex_lock(&AvailableSeatlock);

            int temp = 0;
            //iterating throughout the loop to find out the students who occupied the chairs and pushing them to the queue one by one
            while(temp < numberOfstudents)
            {
                if(QueueForNewStudents[temp] > -1)
                {
                    PriorityQueue[temp][0]=QueueForStudentsPriority[temp];
                    PriorityQueue[temp][1]=QueueForNewStudents[temp];
                    
                    printf("C: Student %d with priority %d added to the queue. Waiting students now = <%d students waiting>. Total requests = <total %d requests (notifications sent) by students for tutoring so far>\n",studentObject->studentID[temp],QueueForStudentsPriority[temp],ChairsOccupied,totalRequests);
                    QueueForNewStudents[temp]= -1;
                    
                    //after students gets picked, pushing them into a queue and waking up tutor to fetch them one by one
                    test = sem_post(&coordinator_waiting_for_tutor);
                    //checking the assert function whether the semaphore passed or not
                    assert(test == 0);
                    temp++;
                }
                temp++;
            }
            
            //unlocking as the chair as student goes out or pushed into the queue leaving the chair 
            pthread_mutex_unlock(&AvailableSeatlock);
        }
    }

    //printf("the coordinator thread terminated\n");
    pthread_exit(NULL);
}

// MAIN METHOD //

int main(int argc, const char * argv[]) {

    // checking whether the arguments passed
    // if(argc != 5){
    //     char errormessage[100] = "Please declare the arguments in this order < csmc #students #tutors #chairs #help >\n";
    //     write(STDERR_FILENO, errormessage, strlen(errormessage));
    //     exit(1);
    // }
    
    //then converting the argv values to integer using atoi function
    // numberOfstudents = atoi(argv[1]);
    // numberOftutors   = atoi(argv[2]);
    // totalNumberOfChairs = atoi(argv[3]);
    // helpsPerPerson =  atoi(argv[4]);

    // test
    numberOfstudents = 6;
    numberOftutors = 3;
    totalNumberOfChairs = 4;
    helpsPerPerson = 5;
    
    //checking the test cases whether the number of tutors passed are greather than the max students or not
    if((numberOfstudents > maxstu) || (numberOftutors > maxtutors))
    { 
       printf("maximum limit has been reached, please check yours arguments assigned\n");
       if(numberOfstudents > maxstu)
       {
            printf("the maximum number of students allowed are %d\n", maxstu);
            exit(1);
       }
       else if(numberOftutors > maxtutors)
       {
           printf("the maximum number of tutors allowed are %d\n", maxtutors);
           exit(1);
       }
       else
       {
            exit(1);
       }
    }
    
    //same goes for the another cases where teh numberof students less than the min students or min tutors
    if((numberOfstudents < minstu) || (numberOftutors < mintutors) || (totalNumberOfChairs < minchairs) || (helpsPerPerson < minhelps))
    {
        char errormessage[100] = "An Error has occured, please check your arguments assigned\n";
        write(STDERR_FILENO, errormessage, strlen(errormessage));
        exit(1);
    }

    //Initalising the semaphores 
    sem_init(&coordinator_waiting_for_tutor, intialiser, intialiser);
    sem_init(&student_waiting_for_coordinator, intialiser, intialiser);

    //Initalising the locks
    pthread_mutex_init(&TutorConfirmation, NULL);
    pthread_mutex_init(&QueueLockByTutor, NULL);
    pthread_mutex_init(&AvailableSeatlock, NULL);
    pthread_mutex_init(&SessionTutoringLock, NULL);

    //calling array function to initalise it
    ArrayIntialiser();
    
    //Initalising the threads
    pthread_t *students;
    pthread_t *tutors;
    pthread_t coordinators[numberofcoordinators];
    

    //Allocating the spaces for the threads
    students = malloc(sizeof(pthread_t) * numberOfstudents);
    tutors   = malloc(sizeof(pthread_t) * numberOftutors);
    
    //Initalising the variables for creating the threads
    int i;
    int j;
    int k;
    
    //Allocating the objects space using the malloc function
    studentObject =  (struct StudentMetaData*)malloc(sizeof(struct StudentMetaData));
    tutorObject =  (struct TutorMetaData*)malloc(sizeof(struct TutorMetaData));

    //Creating the student threads according to the value passed from the argument
    for(i = 0; i < numberOfstudents; i++)
    {
        studentObject->studentID[i] = i + 1;
        if(pthread_create(&students[i], NULL, studentThread, (void*) &studentObject->studentID[i]) != 0)
        {
            perror("Failed to create the student thread");
        }
    }
    
    //Creating the tutor threads according to the value passed from the argument
    for(j = 0; j < numberOftutors; j++)
    {
        tutorObject->tutorID[j] = j + numberOfstudents + 1;
        if(pthread_create(&tutors[j], NULL, TutorThread, (void*) &tutorObject->tutorID[j]) != 0)
        {
            printf("Failed to create the tutor thread");
        }

    }

    //Creating the coordinator threads according to the value from the define
    for(k = 0; k < numberofcoordinators; k++)
    {
     coordinatorID[k] = k + 1; 
     if(pthread_create(&coordinators[k], NULL, CoordinatorThread, (void*) &coordinatorID[k]) != 0)
     {
        perror("Failed to create the thread");
     }

    }
 
    //Waiting for the student thread to join and return a value pointer which will call the exit
    for(j = 0; j < numberOfstudents; j++)
    {
        pthread_join(students[j],NULL);
    }
    
    //Waiting for the tutor thread to join and return a value pointer which will call the exit
    for(i = 0; i < numberOftutors; i++)
    {
        pthread_join(tutors[i],NULL);
    }
    
    //Waiting for the coordinator thread to join and return a value pointer which will call the exit
    for(k = 0; k < numberofcoordinators; k++)
    {
        pthread_join(coordinators[k], NULL);
    }
    
    return 0;
}

