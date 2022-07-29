#include "widget.h"
#include <QApplication>
#include "pthread.h"
#include "unistd.h"
#include "sys/types.h"
#include "sys/sysinfo.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    Widget w;
    w.show();
//    cpu_set_t get;
//    cpu_set_t mask;
//    int num = sysconf(_SC_NPROCESSORS_CONF);
//    pthread_t thread_id = pthread_self();
//    cout << thread_id << endl;
//    CPU_ZERO(&mask);
//    CPU_SET(6, &mask);
//    sched_setaffinity(0, sizeof(mask), &mask);
//    CPU_ZERO(&get);
//    sched_getaffinity(0, sizeof(get), &get);
//    for(int i=0; i<num; ++i)
//    {
//        if(CPU_ISSET(i, &get))
//           cout << "running on " << i << endl;
//    }

    return a.exec();
}
