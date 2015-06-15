#include<stdio.h>
#include<stdlib.h>
#include<fcntl.h>
#include<unistd.h>
#include<time.h>
#include<string.h>
#include<sys/shm.h>
#include<sys/wait.h>

#define ON 1
#define OFF 0

#define EXPORT "/sys/class/gpio/export"

#define LEFT_SENS "/sys/class/gpio/gpio46/"
#define RIGHT_SENS "/sys/class/gpio/gpio27/"
#define LEFT_BUZ "/sys/class/gpio/gpio22/"
#define RIGHT_BUZ "/sys/class/gpio/gpio47/"

void buzz(int trig);
void sona_chk();
void show_time(int time);
void sens_chk();

void init(){
	int fd;
	int i;
	char *tar[] = {"46","27","22","47"};
	
	fd = open(EXPORT,O_WRONLY);
	for(i=0;i<4;i++){
		write(fd,tar[i],strlen(tar[i]));
	}
	fd = open(LEFT_SENS"/direction",O_WRONLY);
	write(fd,"in",2);
	fd = open(RIGHT_SENS"/direction",O_WRONLY);
	write(fd,"in",2);
	fd = open(LEFT_BUZ"/direction",O_WRONLY);
	write(fd,"out",3);
	fd = open(RIGHT_BUZ"/direction",O_WRONLY);
	write(fd,"out",3);
}

int main(){
	int time;
	int time_fd;
	char t_data[20];
	
	init();
	buzz(OFF);
	time_fd = open("./time",O_RDONLY);
	read(time_fd,t_data,4);
	time = atoi(t_data);
	
	if(fork() == 0){
		while(1) show_time(get_time());
	}
	while(time == get_time()) printf("test2\n");
	buzz(ON);
	printf("TEST1\n");
	if(fork() != 0){
		sona_chk();
	}
	else{
		sens_chk();
	}
}

void buzz(int trig) {
	int fd1,fd2;
	
	fd1 = open(LEFT_BUZ"/value",O_WRONLY);
	fd2 = open(RIGHT_BUZ"/value",O_WRONLY);
	if(trig == ON){
		write(fd1,"0",1);
		write(fd2,"0",1);
	}
	else {
		write(fd1,"1",1);
		write(fd2,"1",1);
	}
}

void sens_chk(){
	int fd;
	int shmid;
	int *key;
	char val[1] = "0";

	printf("SENS\n");
	shmid = shmget((key_t)7548,sizeof(int),0666|IPC_CREAT);
	key = (int *)shmat(shmid,NULL,0);
	*key = 0;

	if(fork()){
			while(1){
				do{
					fd = open(LEFT_SENS"/value",O_RDONLY);
					read(fd,val,1);
					*key &= 0xFF;
					usleep(300000);
				}while(val[0] == 0);
				*key |= 0xFF00;
				printf("ls : %c\n",val[0]);
			}
	}
	else{
		fd = open(RIGHT_SENS"/value",O_RDONLY);
			while(1){
				do{
					fd = open(RIGHT_SENS"/value",O_RDONLY);
					read(fd,val,1);
					*key &= 0xFF00; 
					usleep(300000);
				}while(val[0] == 0);

				*key |= 0xFF;
				printf("rs : %c\n",val[0]);
			}
	}
}


int get_time(){
        time_t current;
        struct tm *d;
        current = time(NULL);
        d = localtime(&current);
        return (d->tm_hour*100 + d->tm_min);

}


void show_time(int ntime){
	
}

void sona_chk(){
	int shmid1,shmid2;
	float *distance;
	int *key;
	int cnt = 10;
	
	printf("SONA\n");
	srand(getpid() + time(NULL));
	shmid1 = shmget((key_t)3456,sizeof(float),0666|IPC_CREAT);
	distance = (float *)shmat(shmid1,NULL,0);
	
	shmid2 = shmget((key_t)7548,sizeof(int),0666|IPC_CREAT);
	key = (int *)shmat(shmid2,NULL,0);
	
	if(fork()) system("./hcsr04");
	while(1){
		if(*key == 0xFFFF){
			if(*distance>30){
				usleep(100000);
				printf("sona...%d,%d\n",cnt,*key);
				if(*distance<15) cnt--;
				printf("%d",cnt);
			}
			if(cnt == 0){
				printf("buzz off");
				buzz(OFF);
			}
		}
		else{
		}
	}
}
