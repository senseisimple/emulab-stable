// coprocess.h

/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2003 University of Utah and the Flux Group.
 * All rights reserved.
 */

#ifndef COPROCESS_H_IP_ASSIGN_2
#define COPROCESS_H_IP_ASSIGN_2

class FileWrapper
{
public:
    FileWrapper(FILE * newData = NULL) : data(newData)
    {
    }

    ~FileWrapper()
    {
        if (data != NULL)
        {
            pclose(data);
        }
    }

    FileWrapper(FileWrapper & right) : data(right.data)
    {
        right.data = NULL;
    }

    FileWrapper & operator=(FileWrapper & right)
    {
        FileWrapper temp(right);
        std::swap(data, temp.data);
        return *this;
    }

    void clear(void)
    {
        data = NULL;
    }

    FILE * get(void)
    {
        return data;
    }

    void reset(FILE * newData)
    {
        if (data)
        {
            pclose(data);
        }
        data = newData;
    }

    FILE * release(void)
    {
        FILE * result = data;
        data = NULL;
        return result;
    }
private:
    FILE * data;
};

FileWrapper coprocess(std::string const & command);
int read(FileWrapper & file, int & source, int & dest, int & firstHop,
          int & distance);
void write(FileWrapper & file, char command);
void write(FileWrapper & file, char command, int source, int dest, float cost);

#endif


