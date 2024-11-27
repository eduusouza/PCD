#include <stdio.h>
#include <omp.h>

#define NCLIENTS 1

int main(){
    int i, th_id, respond = 0, request = 0, SOMA;
    omp_set_num_threads(NCLIENTS+1);

#pragma omp parallel private(th_id) shared(SOMA, respond, request)
#pragma omp for    
    th_id = omp_get_thread_num();
    if (th_id ==0){
        while(1){
            while(request == 0){}
            respond = request;
            while(respond != 0){}
            request = 0;
        }

    } else {
        while(1){
            while(respond != i){}
        }
    }
}