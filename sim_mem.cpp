//YousifHibi 208082081
#include <iostream>
#include "sim_mem.h"
#include <unistd.h>
#include <fcntl.h>
using namespace std;
char main_memory[MEMORY_SIZE];
void sim_mem::print_memory() {
    int i;
    printf("\n Physical memory\n");
    for(i = 0; i < MEMORY_SIZE; i++) {
        printf("[%c]\n", main_memory[i]);
    }
}
void sim_mem::print_swap() {
    char* str = (char*)(malloc(this->page_size * sizeof(char)));
    int i;
    printf("\n Swap memory\n");
    lseek(swapfile_fd, 0, SEEK_SET); // go to the start of the file
    while(read(swapfile_fd, str, this->page_size) == this->page_size) {
        for(i = 0; i < page_size; i++) {
            printf("%d - [%c]\t", i, str[i]);
        }
        printf("\n");
    }
}
void sim_mem::print_page_table() {
    int i;
    for (int j = 0; j < num_of_proc; j++) {
        printf("\n page table of process: %d \n", j);
        printf("Valid\t Dirty\t Permission \t Frame\t Swap index\n");
        for(i = 0; i < num_of_pages; i++) {
            printf("[%d]\t[%d]\t[%d]\t[%d]\t[%d]\n",
                   page_table[j][i].V,
                   page_table[j][i].D,
                   page_table[j][i].P,
                   page_table[j][i].frame ,
                   page_table[j][i].swap_index);
        }
    }
}
sim_mem::sim_mem(char exe_file_name1[], char exe_file_name2[], char swap_file_name[],int text_size,
        int data_size, int bss_size, int heap_stack_size,
        int num_of_pages, int page_size, int num_of_process){
    this->page_size=page_size;
    this->num_of_pages=num_of_pages;
    this->text_size=text_size;
    this->data_size=data_size;
    this->bss_size=bss_size;
    this->num_of_proc=num_of_process;
    this->heap_stack_size=heap_stack_size;
    if((text_size+data_size+bss_size+heap_stack_size)/page_size>num_of_pages){
        perror("num of pages error");
        this->sim_mem::~sim_mem();
    }
    if(num_of_process>2||num_of_process<1){
        perror("num of process error");
        this->sim_mem::~sim_mem();
    }
    int swapSize = page_size*(num_of_pages-(text_size/page_size ));
    if((this->program_fd[0] = open(exe_file_name1,O_RDONLY)) <= -1){//open executable file with only read accessibility
        perror( "open executable file Faild");
        this -> sim_mem::~sim_mem();
        exit(EXIT_FAILURE)  ;
    }
    if(num_of_process==2){
    if((this->program_fd[1] = open(exe_file_name2,O_RDONLY)) <= -1){//open executable file with only read accessibility
        perror( "open executable file Faild");
        this -> sim_mem::~sim_mem();
        exit(EXIT_FAILURE)  ;
    }}
    if((this->swapfile_fd= open(swap_file_name, O_CREAT| O_RDWR ,0666 )) <= -1){//open swap file with the ability to read and wright  and creat if the file dose not exists
        perror( "open swape file Faild");
        this -> sim_mem::~sim_mem();
        exit(EXIT_FAILURE) ;
    }
    for(int i = 0; i < swapSize ; i++) {// but zeros in the swap file
        if (write(this->swapfile_fd, "0", 1) == -1) {
            fprintf(stderr, "Write Faild");
            this -> sim_mem::~sim_mem();
            exit(1);
        }
    }
    sim_mem::page_table = new page_descriptor*[num_of_process];// identify the page table
    for (int i = 0; i <num_of_process; i++) {
        sim_mem::page_table [i] = new page_descriptor[num_of_pages];

    for(int  j=0 ; j<num_of_pages ; j++){
        page_table[i][j].V=0;
        page_table[i][j].frame=-1;
        page_table[i][j].swap_index =-1;
        if(j < (text_size / page_size)) {
            page_table[i][j].P= 0;
        }else page_table[i][j].P = 1;
        page_table[i][j].D = 0;
    }
    }
    for (int i = 0; i < MEMORY_SIZE; ++i) {
        main_memory[i]='0';
    }
}
char sim_mem::load(int process_id, int address){
    if(address<0||process_id<1||process_id>2){
        return '\0';
    }
    int page= address/page_size;
    int offset=address%page_size;
    int frame;
    process_id--;
    if(page>num_of_pages){
        perror("address is out of range");
        return '\0';
    }
    if(page_table[process_id][page].V==0){
        if(page_table[process_id][page].P==0){

        page_table[process_id][page].V=1;
        searchMemory(process_id,page);
        butInMemory(process_id,page);
        frame=  page_table[process_id][page].frame;
            return main_memory[(frame*page_size)+offset];
        }
        if(page_table[process_id][page].P==1){
            if(page_table[process_id][page].D==0){
                if((text_size+data_size)>=address){
                    page_table[process_id][page].V=1;
                    searchMemory(process_id,page);
                    butInMemory(process_id,page);
                    frame=  page_table[process_id][page].frame;
                    return main_memory[(frame*page_size)+offset];
                }
                else{
                    perror("can't load to bss or heap-stack ");
                    return '\0';
                }
            }
            if(page_table[process_id][page].D==1){
                char *s = new char [page_size];
                    lseek(swapfile_fd,page_table[process_id][page].swap_index*page_size,SEEK_SET);
                    read(swapfile_fd,s,page_size);
                for (int i = 0; i < page_size; ++i) {
                    main_memory[i+(page_table[process_id][page].frame*page_size)]=s[i];
                    if (write(swapfile_fd, "0", 1) == -1) {
                        fprintf(stderr, "Write Faild");
                        exit(1);
                    }
                    lseek(swapfile_fd,page_table[process_id][page].swap_index*page_size+i,SEEK_SET);
                }
                if (write(swapfile_fd, "0", 1) == -1) {
                    fprintf(stderr, "Write Faild");
                    exit(1);
                }
                page_table[process_id][page].V=1;
                page_table[process_id][page].D=0;
                delete [] s;
                return main_memory[(frame*page_size)+offset];
            }
        }
    }if(page_table[process_id][page].V==1){
        frame=  page_table[process_id][page].frame;
        return main_memory[(frame*page_size)+offset];
    }
    return '\0';
}
void sim_mem::store(int process_id, int address, char value){
    if(address<0||process_id<1||process_id>2){
        return ;
    }
    process_id--;
    int page= address/page_size;
    int offset=address%page_size;
    int frame;
    if(page>num_of_pages){
        perror("address is out of range");
        return ;
    }
    if(page_table[process_id][page].P==1){
    if(page_table[process_id][page].V==1){
        butInswap(process_id,page);
        page_table[process_id][page].V=0;
        page_table[process_id][page].D=1;
        main_memory[offset + (page_table[process_id][page].frame * page_size)]=value;
    } if(page_table[process_id][page].V==0){
        if(page_table[process_id][page].D==0){
            searchMemory(process_id,page);
            butInMemory(process_id,page);
            main_memory[offset + (page_table[process_id][page].frame * page_size)]=value;
            page_table[process_id][page].D=1;
        }if(page_table[process_id][page].D==1){
                butInswap(process_id,page);
                main_memory[offset + (page_table[process_id][page].frame * page_size)]=value;
        }
    }
    }if(page_table[process_id][page].P==0){
        perror("erorr, can not store to  text");
        return;
    }

}
void sim_mem::butInswap(int process_id, int page){
    lseek(swapfile_fd,0,SEEK_SET);
    char* str=new char[page_size];
    bool b= false;
    int s = 0;
    int swapSize = page_size*(num_of_pages-(text_size/page_size ));
    for ( s = 0; s < swapSize; s=s+page_size) {
        lseek(swapfile_fd,s,SEEK_CUR);
        read(swapfile_fd,str,page_size);
        b=true;
        for (int i = 0; i < page_size; ++i) {
            if(str[i]!='0')
            b= false;
        }
        if(b==true){
            break;
        }
    }
    if(b==true) {
        lseek(swapfile_fd,s,SEEK_SET);
        page_table[process_id][page].swap_index=s/page_size;
        char *c=new char[page_size];
        for (int i = 0; i < page_size; i++) {// put zeros in the swap file
           c[i] = (main_memory[i + (page_table[process_id][page].frame * page_size)]);
        }
        if (write(swapfile_fd, c, page_size) == -1) {
            fprintf(stderr, "Write Faild");
            exit(1);
        }
    }
    if (b== false){
      int r = (rand() % swapSize )/page_size;
        for (int i = 0; i < num_of_proc; ++i) {
            for (int j = 0; j <num_of_pages ; ++j) {
                if(page_table[i][j].swap_index==r){
                page_table[i][j].swap_index=-1;
            }
        }
        }
        lseek(swapfile_fd,r*page_size,SEEK_SET);
        page_table[process_id][page].swap_index=r;
        char *c=new char[page_size];
        for (int i = 0; i < page_size; i++) {// but zeros in the swap file
            c[i] = (main_memory[i + (page_table[process_id][page].frame * page_size)]);
        }
        if (write(swapfile_fd, c, page_size) == -1) {
            fprintf(stderr, "Write Faild");
            exit(1);
        }
    }
}
void sim_mem::searchMemory(int process_id, int page){
    bool b= true;
    for (int i = 0; i < MEMORY_SIZE; i=i+page_size) {
        if(main_memory[i]=='0'){
            b= true;
            for (int j = 0; j < page_size; ++j) {
                if(main_memory[i+j]!='0'){
                    b= false;
                }
            }
            if(b==true){
                page_table[process_id][page].frame=i/page_size;
                break;
            }
        }
    }
    if(b== false){
        FIFO();
        searchMemory( process_id,  page);
    }
}
void sim_mem::FIFO() {

    for (int i = 0; i < MEMORY_SIZE; ++i) {
        if (i+page_size<MEMORY_SIZE){
        main_memory[i]=main_memory[page_size+i];
        }
        else{
            main_memory[i]='0';
        }
    }

    for (int i = 0; i < num_of_proc; ++i) {
        for (int j = 0; j <num_of_pages ; ++j) {
            if(page_table[i][j].frame!=0){
            if(page_table[i][j].frame!=-1){
            page_table[i][j].frame--;}}
            if(page_table[i][j].frame==0){
                page_table[i][j].frame=-1;
                page_table[i][j].V=0;
              }

        }
    }
}
void sim_mem::butInMemory(int process_id, int page){
    char *s = new char [page_size];
    if((text_size+data_size)>=page*page_size){
    lseek(program_fd[process_id],page*page_size,SEEK_SET);
    read(program_fd[process_id],s,page_size);
    }
    else{
        for (int i = 0; i < page_size; ++i) {
            s[i]='0';
        }
    }
    for (int i = 0; i < page_size; ++i) {
        main_memory[i+(page_table[process_id][page].frame*page_size)]=s[i];
    }
    delete [] s;
}
sim_mem::~sim_mem(){
    if(close(swapfile_fd)==-1 || close(program_fd[0])==-1) {
        perror("close Filed");
        exit(EXIT_FAILURE);
    }
    if(num_of_proc==2){
        if( close(program_fd[1])==-1) {
            perror("close Filed");
            exit(EXIT_FAILURE);
        }
    }
    delete [] page_table;
}
