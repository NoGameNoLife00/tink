#ifndef TINK_DATA_BUFFER_H
#define TINK_DATA_BUFFER_H

namespace tink {
    struct message {
        char * buffer;
        int size;
    };
    struct data_buffer {
        int header;
        int offset;
        int size;
    };


}




#endif //TINK_DATA_BUFFER_H
