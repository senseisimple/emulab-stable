// coprocess.cc

/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2003 University of Utah and the Flux Group.
 * All rights reserved.
 */

#include <unistd.h>

#include "lib.h"
#include "coprocess.h"

using namespace std;

FileWrapper coprocess(string const & command)
{
    FileWrapper temp(popen(command.c_str(), "r+"));
    return temp;
}

int read(FileWrapper & file, int & source, int & dest, int & firstHop,
          int & distance)
{
    int count = 0;
    count += fread(&source, sizeof(source), 1, file.get());
    count += fread(&dest, sizeof(dest), 1, file.get());
    count += fread(&firstHop, sizeof(firstHop), 1, file.get());
    count += fread(&distance, sizeof(distance), 1, file.get());
    return count;
}

void write(FileWrapper & file, char command)
{
    fwrite(&command, sizeof(command), 1, file.get());
}

void write(FileWrapper & file, char command, int source, int dest, float cost)
{
    fwrite(&command, sizeof(command), 1, file.get());
    fwrite(&source, sizeof(source), 1, file.get());
    fwrite(&dest, sizeof(dest), 1, file.get());
    fwrite(&cost, sizeof(cost), 1, file.get());
}
