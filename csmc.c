#include <pthread.h>
#include <unistd.h>
#include <stdlib.h>
#include <semaphore.h>
#include <stdio.h>
#include <time.h>
#include <string.h>

#define maxarg 5
#define maxstu 2000
#define maxtutors 2000
#define maxchairs 20
#define minchairs 1
#define minstu 1
#define mintutors 1
#define minhelps 1
#define maxqueue 2000
#define maxcoordinator 1
#define intialiser 0


int RequestsDone = 0;
int totalRequests = 0;
int CompleteSessions = 0;
int SessionTutoring = 0;
int itr;

struct StudentMetaData
{
    int studentID[maxstu];
};

struct TutorMetaData
{
    int tutorID[maxstu];
};

struct StudentMetaData* studentObject;
struct TutorMetaData* tutorObject;

void *studentThread(void *student);
void *TutorThread(void *tutor);
void *CoordinatorThread(void *coordinator);


int numberOfstudents;
int numberOftutors;
int helpsPerPerson;
int totalNumberOfChairs;
int ChairsOccupied = 0;
int numberofcoordinators = 1;

int QueueForNewStudents[maxstu];
int QueueForTutors[maxstu];
int PriorityQueue[maxstu][2];
int QueueForStudentsPriority[maxstu];
int coordinatorID[maxcoordinator];

sem_t student_waiting_for_coordinator;
sem_t coordinator_waiting_for_tutor;

pthread_mutex_t AvailableSeatlock;
pthread_mutex_t QueueLockByTutor;
pthread_mutex_t TutorConfirmation;
pthread_mutex_t SessionTutoringLock;

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

void *studentThread(void *student){
    int studentrecord=*(int*)student;
    
    while(1){
        if(QueueForStudentsPriority[studentrecord-1]>=helpsPerPerson) {
            
            pthread_mutex_lock(&AvailableSeatlock);
            RequestsDone = RequestsDone + 1;
            pthread_mutex_unlock(&AvailableSeatlock);

            sem_post(&student_waiting_for_coordinator);
    
            pthread_exit(NULL);
        }
        
        else
        {
            if(CasesWorking())
            {
                int random = randomfunction(intialiser);
                usleep(rand() % random);           
                pthread_mutex_lock(&AvailableSeatlock);
                if(ChairsOccupied >= totalNumberOfChairs){
                    printf("S: Student %d found no empty chair. Will try again later.\n",studentrecord);
                    printf("The total number of requests till now: %d\n", totalRequests);
                    pthread_mutex_unlock(&AvailableSeatlock);
                    continue;
                }

                totalRequests  = totalRequests + 1 ;
                ChairsOccupied = ChairsOccupied + 1;
            
                QueueForNewStudents[studentrecord-1] = totalRequests;
                

                printf("S: Student %d takes a seat. Empty chairs = <%d of empty chairs after student %d took a seat>.\n",studentrecord,totalNumberOfChairs-ChairsOccupied, studentrecord);
                pthread_mutex_unlock(&AvailableSeatlock);
                
                sem_post(&student_waiting_for_coordinator);
                
                while(QueueForTutors[studentrecord-1] == -1);

                printf("S: Student %d received help from Tutor %d.\n",studentrecord,QueueForTutors[studentrecord-1]-numberOfstudents);
                
                pthread_mutex_lock(&TutorConfirmation);
                QueueForTutors[studentrecord-1]=-1;
                pthread_mutex_unlock(&TutorConfirmation);
                
                pthread_mutex_lock(&AvailableSeatlock);
                QueueForStudentsPriority[studentrecord-1]++;
                pthread_mutex_unlock(&AvailableSeatlock);
            }
            else
            {
                pthread_exit(NULL);
            }
        }
    }
}

void *TutorThread(void *tutor){
    int tutrID=*(int*)tutor;
    int TimeTakenByTutor;

    int StudentTime;
    int studentrecord;
    
    while(1){

        if(RequestsDone == numberOfstudents){

            pthread_exit(NULL);
        }
        
        TimeTakenByTutor = helpsPerPerson - 1;
        StudentTime = numberOfstudents * helpsPerPerson + 1;
        studentrecord=-1;
        

        sem_wait(&coordinator_waiting_for_tutor);
        
        pthread_mutex_lock(&AvailableSeatlock);

        int temp;
        for(temp=0;temp<numberOfstudents;temp++){
            if(PriorityQueue[temp][0]>-1 && PriorityQueue[temp][0]<=TimeTakenByTutor
               && PriorityQueue[temp][1]<StudentTime){
                TimeTakenByTutor=PriorityQueue[temp][0];
                StudentTime = PriorityQueue[temp][1];
                studentrecord = studentObject->studentID[temp];
            }
        }
        

        if(studentrecord == -1) {
            pthread_mutex_unlock(&AvailableSeatlock);
            continue;
        }
        
        PriorityQueue[studentrecord-1][0]=-1;
        PriorityQueue[studentrecord-1][1]=-1;
        
        pthread_mutex_lock(&SessionTutoringLock);
        SessionTutoring = SessionTutoring + 1;
        pthread_mutex_unlock(&SessionTutoringLock);

        ChairsOccupied = ChairsOccupied - 1;

        pthread_mutex_unlock(&AvailableSeatlock);

        int random2 = randomfunction(mintutors);   
        usleep(rand() % random2);
        

        pthread_mutex_lock(&AvailableSeatlock);

        CompleteSessions = CompleteSessions + 1;

        pthread_mutex_lock(&SessionTutoringLock);
        SessionTutoring = SessionTutoring - 1;
        pthread_mutex_unlock(&SessionTutoringLock);
        
        printf("T: Student %d tutored by Tutor %d. Students tutored now = <%d students receiving help now>. Total sessions tutored = <%d total number of tutoring sessions completed so far by all the tutors>\n",studentrecord,tutrID-numberOfstudents,SessionTutoring,CompleteSessions);
        pthread_mutex_unlock(&AvailableSeatlock);

        pthread_mutex_lock(&TutorConfirmation);
        QueueForTutors[studentrecord-1] = tutrID;
        pthread_mutex_unlock(&TutorConfirmation);
        
    }
}


void *CoordinatorThread(void *coordinator){
    int coodID = * (int*)coordinator;
    while(1){

        if(RequestsDone == numberOfstudents){

            int temp;
            for(temp=0;temp<numberOftutors;temp++){

	        sem_post(&coordinator_waiting_for_tutor);
            }
            
            pthread_exit(NULL);
        }

        sem_wait(&student_waiting_for_coordinator);
        
        pthread_mutex_lock(&AvailableSeatlock);

        int temp;
        for(temp = 0;temp < numberOfstudents;temp++){
            if(QueueForNewStudents[temp]>-1){
                PriorityQueue[temp][0]=QueueForStudentsPriority[temp];
                PriorityQueue[temp][1]=QueueForNewStudents[temp];
                
                printf("C: Student %d with priority %d added to the queue. Waiting students now = <%d students waiting>. Total requests = <total %d requests (notifications sent) by students for tutoring so far>\n",studentObject->studentID[temp],QueueForStudentsPriority[temp],ChairsOccupied,totalRequests);
                QueueForNewStudents[temp]= -1;
                
                sem_post(&coordinator_waiting_for_tutor);
            }
        }
        pthread_mutex_unlock(&AvailableSeatlock);
    }
}

int main(int argc, const char * argv[]) {


    // if(argc != 5){
    //     char errormessage[100] = "Please declare the arguments in this order < csmc #students #tutors #chairs #help >\n";
    //     write(STDERR_FILENO, errormessage, strlen(errormessage));
    //     exit(1);
    // }

    // int numberOfstudents = atoi(argv[1]);
    // int numberOftutors   = atoi(argv[2]);
    // int totalNumberOfChairs = atoi(argv[3]);
    // int helpsPerPerson =  atoi(argv[4]);
    // int numberOfCoordinator = 1;

    numberOfstudents = 6;
    numberOftutors = 3;
    totalNumberOfChairs = 4;
    helpsPerPerson = 5;
    
    if((numberOfstudents > maxstu) || (numberOftutors > maxtutors))
    { 
       printf("maximum limit has been reached, please check yours arguments assigned\n");
       printf("the maximum number of students allowed are %d\n", maxstu);
       printf("the maximum number of tutors allowed are %d\n", maxtutors);
       exit(1);
    }

    if((numberOfstudents < minstu) || (numberOftutors < mintutors) || (totalNumberOfChairs < minchairs) || (helpsPerPerson < minhelps))
    {
        char errormessage[100] = "An Error has occured, please check your arguments assigned\n";
        write(STDERR_FILENO, errormessage, strlen(errormessage));
        exit(1);
    }

    //init lock and semaphore
    sem_init(&coordinator_waiting_for_tutor, intialiser, intialiser);
    sem_init(&student_waiting_for_coordinator, intialiser, intialiser);

    pthread_mutex_init(&TutorConfirmation, NULL);
    pthread_mutex_init(&QueueLockByTutor, NULL);
    pthread_mutex_init(&AvailableSeatlock, NULL);
    pthread_mutex_init(&SessionTutoringLock, NULL);

    itr = 0;
    while(itr<numberOfstudents)
    {
        if(CasesWorking())
        {
            QueueForNewStudents[itr] = -1;
            QueueForTutors[itr] = -1;
            PriorityQueue[itr][0] = -1;
            PriorityQueue[itr][1] = -1;
            QueueForStudentsPriority[itr]=0;
            itr++;
        }
        else
        {
            itr++;
        }
    }
    
    pthread_t *students;
    pthread_t *tutors;
    pthread_t coordinators[numberofcoordinators];
    
    students = malloc(sizeof(pthread_t) * numberOfstudents);
    tutors   = malloc(sizeof(pthread_t) * numberOftutors);

    int i;
    int j;
    int k;
    
    studentObject =  (struct StudentMetaData*)malloc(sizeof(struct StudentMetaData));
    tutorObject =  (struct TutorMetaData*)malloc(sizeof(struct TutorMetaData));


    
    for(i = 0; i < numberOfstudents; i++)
    {
        studentObject->studentID[i] = i + 1;
        if(pthread_create(&students[i], NULL, studentThread, (void*) &studentObject->studentID[i]) != 0)
        {
            perror("Failed to create the student thread");
        }
    }
    
    for(j = 0; j < numberOftutors; j++)
    {
        tutorObject->tutorID[j] = j + numberOfstudents + 1;
        if(pthread_create(&tutors[j], NULL, TutorThread, (void*) &tutorObject->tutorID[j]) != 0)
        {
            printf("Failed to create the tutor thread");
        }

    }

    for(k = 0; k < numberofcoordinators; k++)
    {
     coordinatorID[k] = k + 1; 
     if(pthread_create(&coordinators[k], NULL, CoordinatorThread, (void*) &coordinatorID[k]) != 0)
     {
        perror("Failed to create the thread");
     }

    }
 
    for(j = 0; j < numberOfstudents; j++)
    {
        pthread_join(students[j],NULL);
    }
    
    for(i = 0; i < numberOftutors; i++)
    {
        pthread_join(tutors[i],NULL);
    }

    for(k = 0; k < numberofcoordinators; k++)
    {
        pthread_join(coordinators[k], NULL);
    }
    
    return 0;
}

