/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2000-2005 University of Utah and the Flux Group.
 * All rights reserved.
 */

int getnumcpus(void);
int getcpuspeed(void);
int getmempages(void);
char **getdrivedata(char**);

int *getmembusy(unsigned pages);
int *getnetmembusy(void);
int getdiskbusy(void);

int *getcpustates();

int procpipe(char *const prog[], int (procfunc)(char*,void*), void* data);
