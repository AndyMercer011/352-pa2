#include <zconf.h>
#include <stdio.h>
#include <stdlib.h>
#include <semaphore.h>
#include <pthread.h>
#include <ctype.h>


sem_t input_sem_read,input_sem_write;
sem_t output_sem_read,output_sem_write;
sem_t input_sem_counter;
sem_t output_sem_counter;

int input_flag;
int output_flag;

int input_size,output_size;

int input_count_result[26],output_count_result[26];

char* input_buffer;
char* output_buffer;

FILE * input_file;
FILE * output_file;

int buffer_size;

pthread_mutex_t mutex;

void * reader_thread(void * ptr);
void * writer_thread(void * ptr);
void * encrypt_thread(void * ptr);
void * input_counter_thread(void * ptr);
void * output_counter_thread(void * ptr);

int main(int argc, const char * argv[]){
	buffer_size = 0;
	pthread_t tid[5];
	input_size = 0;
	output_size = 0;
	input_flag = 0;
	output_flag = 0;
	pthread_mutex_init (&mutex,NULL);
	

	printf("Enter the buffer Size:");
    while (scanf("%d",&buffer_size) == 0){
        printf("please type in an integer.\n");
        printf("Enter the buffer Size:");  
    }
    //printf("p start1\n");
    
    sem_init(&input_sem_read, 0, 0);
    sem_init(&output_sem_read, 0, 0);
    sem_init(&input_sem_write, 0, buffer_size);
    sem_init(&output_sem_write, 0, buffer_size);
    sem_init(&input_sem_counter, 0, 0);
    sem_init(&output_sem_counter, 0, 0);

    input_file = fopen(argv[1],"r");
    output_file = fopen(argv[2],"w");
    
    input_buffer = calloc(buffer_size, sizeof(char));
    output_buffer = calloc(buffer_size, sizeof(char));
    pthread_create(&tid[0],NULL,reader_thread,NULL);
    pthread_create(&tid[1],NULL,writer_thread,NULL);
	pthread_create(&tid[2],NULL,encrypt_thread,NULL);
	pthread_create(&tid[3],NULL,input_counter_thread,NULL);
    pthread_create(&tid[4],NULL,output_counter_thread,NULL);
    
    for (int k = 0; k < 5; ++k) {
        pthread_join(tid[k],NULL);
    }
    
    printf("Input buffer contains:\n");
    for (int i = 0; i < 26; ++i){
        if(input_count_result[i] > 0)
            printf("%c: %d\n",'A' + i,input_count_result[i]);

    }
    printf("Output file contains:\n");
    for (int j = 0; j < 26; ++j){
        if(output_count_result[j])
            printf("%c: %d\n",'A' + j,output_count_result[j]);
    }
    
    sem_destroy(&input_sem_read);
    sem_destroy(&input_sem_write);
    sem_destroy(&output_sem_read);
    sem_destroy(&output_sem_write);
    sem_destroy(&input_sem_counter);
    sem_destroy(&output_sem_counter);

    free(input_buffer);
    free(output_buffer);
	
}

void * reader_thread(void * ptr){
    char buffer;
        

    fread(&buffer,1,1,input_file);
    //pthread_mutex_lock (&mutex);

    while (input_flag != 1){
    	//printf("read begin\n");
    	//pthread_mutex_lock (&mutex);
        //printf("read start\n");
        sem_wait(&input_sem_write);
        //block write, start to read
        //printf("read in process\n");

        input_buffer[input_size % buffer_size] = buffer;
        //printf("read in process\n");

        fread(&buffer,1,1,input_file);
        //printf("read in process\n");

        if(feof(input_file))
            input_flag = 1;
        else
            input_size++;
            
		//allow counter to leading in, open next read
        sem_post(&input_sem_counter);

        sem_post(&input_sem_read);
        
        //pthread_mutex_unlock (&mutex);
        //printf("read finished\n");
    }

    pthread_exit(NULL);
}

void * writer_thread(void * ptr){
    int flag = 0;
    int head_index = 0;
    
        


    while (flag != 1){
    	//pthread_mutex_lock (&mutex);
    	
    	//lock read, start write into outfile
        sem_wait(&output_sem_read);
        //printf("write begin\n");

        fwrite(&output_buffer[head_index % buffer_size],1,1,output_file);

        if(output_flag == 1 && head_index == output_size)
            flag = 1;
        else
            head_index++;
		//allow counter leading in, open next write.
        sem_post(&output_sem_counter);
        

        sem_post(&output_sem_write);
        //pthread_mutex_unlock (&mutex);
    }
    fclose(output_file);
    pthread_exit(NULL);
}

void * encrypt_thread(void * ptr){
    int flag = 1;
        

    while(output_flag != 1){
    	
    	//block read and write ability during encrypt
        sem_wait(&input_sem_read);
        //printf("encrypt begin\n");

        sem_wait(&output_sem_write);

        char tmp = ' ';
        char original = input_buffer[output_size % buffer_size];

        if((original >= 'A' && original <= 'Z')||(original >= 'a' && original <= 'z')){
           	if(flag == 1){
           		if(original == 'Z'){
           			tmp = 'A';
				   }
				   else if (original == 'z'){
				   	tmp = 'a';
				   }
				   else 
				   tmp = (u_int)original + 1;
				   flag = -1;
			}
			else if(flag == -1){
				if(original == 'A'){
           			tmp = 'Z';
				   }
				   else if (original == 'a'){
				   	tmp = 'z';
				   }
				   else 
				   tmp = (u_int)original - 1;
				   flag = 0;
			}
			else{
				tmp = original;
				flag = 1;
			}           
        }else
            tmp = original;

        output_buffer[output_size % buffer_size] = tmp;

        if(input_flag == 1 && input_size == output_size)
            output_flag = 1;
        else
            output_size++;
		//reopen the write and read, let next char in
        sem_post(&input_sem_write);

        sem_post(&output_sem_read);
    }
    pthread_exit(NULL);
}

void * input_counter_thread(void * ptr){
    int flag = 0;
    int index = 0;
    

    while (flag != 1){
        sem_wait(&input_sem_counter);
        //printf("input count begin\n");
		//reading through the buffer to adding point to result
        if(isalpha(input_buffer[index % buffer_size]))
            input_count_result[toupper(input_buffer[index % buffer_size]) - 'A'] ++;

        if(input_flag == 1 && index == input_size)
            flag = 1;
        else
            index++;

    }
    pthread_exit(NULL);
}

void * output_counter_thread(void * ptr){
    int flag = 0;
    int index = 0;


    while (flag != 1){
        sem_wait(&output_sem_counter);
        //printf("output count begin\n");
		//reading through the buffer to adding point to result
        if(isalpha(output_buffer[index % buffer_size]))
            output_count_result[toupper(output_buffer[index % buffer_size]) - 'A'] ++;

        if(output_flag == 1 && index == output_size)
            flag = 1;
        else
            index++;

    }
    pthread_exit(NULL);
}


