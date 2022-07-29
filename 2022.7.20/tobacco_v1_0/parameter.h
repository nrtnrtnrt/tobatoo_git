#ifndef PARAMETER_H
#define PARAMETER_H

#include "camera.h"
#include <QTime>




class Parameter{
public:
    QTime lamp_timer;
    uint32_t lamp_used_time = 0;
    string password;
    vector<string> file_info = vector<string>(2);
    uint32_t valve_cnt_vector[256] = {0};
    QDateTime current_time;
};




#endif // PARAMETER_H
