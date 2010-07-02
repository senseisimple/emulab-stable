/*
 * dynamicBuild.cc
 * Copyright (C) 1999 Norio Katayama
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free
 * Software Foundation, Inc., 59 Temple Place - Suite 330, Boston,
 * MA 02111-1307, USA
 *
 * 10/14/96 katayama@rd.nacsis.ac.jp
 * $Id: wvcluster.cc,v 1.1 2007-09-14 21:40:16 pramod Exp $
 */

/* Modified into wvcluster.cc  by Taekhyun Kim */
/* 2005.07.03 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fstream.h>
#include <vector>
#include <algorithm>

#ifdef _MSC_VER
#include "HnSRTree/HnGetOpt.h"
#else
#include <unistd.h>
#endif
#include "RecordFileSt.hh"
#include "HnSRTree/HnSRTreeFile.hh"

#include "HnSRTree/HnStringBuffer.hh"
#include "HnSRTree/HnTimesSt.h"


#define DEFAULT_ATTRIBUTE_SIZE 64
#define DEFAULT_THRESHOLD 0.750
#define NDATA   100
#define NDIM  5 

typedef struct{
    double Ocount;
    double FalsePOS;
    double FalseNEG;
    double CPUtime;
} WVresultSt;

typedef struct{
    double Ocount;
    double FalsePOS;
    double FalseNEG;
} CAresultSt;

static HnTimesSt *ST1, *ST2, *ET1,*ET2, *RT1, *RT2;
static HnSRTreeProfileSt *treeProfile;

static void printUsage(void);
static WVresultSt wvcluster(const char *recordFileName, int maxCount,
        const char *indexFileName, int dataItemSize,
        const HnProperties &properties, HnBool debug,
        HnBool onMemory,double threshold,int verbose,const char *resultFileName, char *congFileName);
CAresultSt ClusterAccuracy(vector<vector<int> > Clustered,int CMcount,char *congFileName,int verbose);

/* profiling */
static void initializeProfileData(void);
static void startProfiling(int);
static void endProfiling(int);
static void printProfileData(int);
static void freeProfileData(void);

    int
main(int argc, char *argv[])
{
    //char recordFileName[64]; 
    //char indexFileName[64];
    char *recordFileName; 
    char *indexFileName;
    char congFileName[64];
    char resultFileName[64];
    char buff[16];
    int dataItemSize;
    int maxCount;
    double CATotal,FalsePOSTotal,FalseNEGTotal;
    int verbose;
    double CPUtimeTotal;
    int dimensions[5]={18,36,61,101,170};
    int npaths[7]={1,2,4,8,16,32,64};
    WVresultSt WVresult;
    HnBool debug;
    HnBool onMemory;
    HnStringBuffer sb;
    HnProperties properties;

    int i,j,c, errflag;
    extern int optind;
    extern char *optarg;
    int dim = 18;

    double threshold;

    WVresult.Ocount=0;
    WVresult.CPUtime=0;

    dataItemSize = DEFAULT_ATTRIBUTE_SIZE;
    threshold = DEFAULT_THRESHOLD;
    maxCount = -1;
    debug = HnFALSE;
    onMemory = HnFALSE;
    verbose = 0;

    sb = new_HnStringBuffer();

    errflag = 0;
    /*
    while ( (c = getopt(argc, argv, "c:dmp:v:s:t:i:")) != EOF ) {
        switch ( c ) {
            case 'c':
                maxCount = atoi(optarg);
                break;
            case 'd':
                debug = HnTRUE;
                break;
            case 'm':
                onMemory = HnTRUE;
                break;
            case 'p':
                sb.append(optarg);
                sb.append('\n');
                break;
            case 'v':
                verbose = atoi(optarg);
                break;
            case 's':
                dataItemSize = atoi(optarg);
                break;
            case 't':
                threshold = atof(optarg);
                if(verbose)
                    printf("threshold = %f\n",threshold);
                break;
            case 'i':
                dim=atoi(optarg);
                break;
            case '?':
                errflag = 1;
                break;
        }
    }
    */
    /*
    if ( errflag || optind != argc ) {
        printUsage();
        return 1;
    }
    */

    maxCount = 10000;
    dim = 16;
    verbose = 0;
    //recordFileName = argv[optind];
    //indexFileName = argv[optind + 1];
    recordFileName = argv[1];
    indexFileName = argv[2];

    properties = new_HnProperties();
    properties.load(sb.toString());

    strcpy(resultFileName, "result.file");

    WVresult =  wvcluster(recordFileName, maxCount,
            indexFileName, dataItemSize,
            properties, debug, onMemory,threshold,verbose,resultFileName,congFileName);

    return 0;
}

    static void
printUsage(void)
{
    fprintf(stderr, "\
            Usage: wvcluster [options] recordFile indexFile\n\
            Options\n\
            -c count     set the number of records to be read from recordFile\n\
            -d           turn on the debug mode\n\
            -m           build the tree on memory and finally dump it to the file\n\
            -p property  set the property of the index\n\
            -s dataSize  set the size of data attributes (%d by default)\n\
            -t threshold set the threshold for XCOR test (%f by default)\n\
            ", DEFAULT_ATTRIBUTE_SIZE,DEFAULT_THRESHOLD);
}

static WVresultSt 
wvcluster(const char *recordFileName, int maxCount,
        const char *indexFileName, int dataItemSize,
        const HnProperties &properties, HnBool debug,
        HnBool onMemory,double threshold,int verbose,const char *resultFileName, char *congFileName)
{
    RecordFileSt *recordFile;
    HnSRTreeFile indexCM;
    HnPointVector allPaths;
    //HnPointVector allPaths2;
    HnDataItemVector allData;
    int count;
    int i1,i2;
    HnProperties treeProperties;
    WVresultSt Result;
    CAresultSt CAresult;

    vector<vector<int> > Group(1024);
    vector<double> CPUtimes1;
    vector<double> CPUtimes2;

    ofstream resultFile(resultFileName);

    HnSRTreeFile::setDebug(debug);

    initializeProfileData();

    recordFile = RecordFileSt_open(recordFileName);
    indexCM = new_HnSRTreeFile((char *)NULL ,
            recordFile->dimension, dataItemSize, 
            properties);

    if ( indexCM == HnSRTreeFile::null ) {
        perror(indexFileName);
        exit(1);
    }

    treeProperties = indexCM.getProperties();
    //treeProperties.print();
    indexCM.initializeStore();

    startProfiling(2);
    allPaths = new_HnPointVector();
    allData = new_HnDataItemVector();
    count = 0;

    /* Inserting record points to index file */

    while ( maxCount < 0 || count < maxCount ) {
        HnPoint point;
        HnDataItem dataItem;

        if ( RecordFileSt_getRecord(recordFile, &point, &dataItem) < 0 ) {
            break;
        }
        //printf("Inserting path %s Clustered(%d) \n",dataItem.toCharArray(),point.isClustered());
        //indexFile.store(point, dataItem);

        double *tmpArray = point.getCoords();

        //for(int i = 0;i < point.getDimension(); i++)
        //{
        //    printf("Coord[%d] = %f\n", i, tmpArray[i]);
        //}
        allPaths.addElement(point);
        allData.addElement(dataItem);


        count ++;
    }

    /* End of inserting */

    /* Clustering */

    //indexCM = new_HnSRTreeFile( (char *)NULL ,
    //			 recordFile->dimension, dataItemSize, 
    //			 properties);

    float radius;
    double totalTimes = 0.0;
    int CMcount = 0;

    radius = sqrt(2-2*threshold);
    if(verbose)
        printf("Radius = %f \n",radius);


    for(i1=0;i1<allPaths.size();i1++)   
    {

        HnPoint querypoint;
        HnDataItem querydata;
        HnPoint newCM;
        HnDataItem newCMdata;
        char CMdatabuf[10];

        HnPointVector CMpoints;
        HnDataItemVector CMdataItems;

        querypoint = allPaths.elementAt(i1);
        querydata = allData.elementAt(i1);

        //startProfiling(1);
        indexCM.getNeighbors(querypoint,1,&CMpoints,&CMdataItems);
        //endProfiling(1);
        //totalTimes = totalTimes + HnTimesSt_getCPUTime(RT1);
        //CPUtimes1.push_back(totalTimes);

        if(CMpoints.size()==0 )
        {

            newCM=new_HnPoint(querypoint);

            CMcount++;

            sprintf(CMdatabuf,"%d",CMcount-1);
            newCMdata = new_HnDataItem(CMdatabuf,10);
            char tmpStr[100] = "", tmpStr2[100];

            strcpy(tmpStr2, querydata.toCharArray());
            tmpStr2[strlen(tmpStr2-2)] = '\0';
            strcpy(tmpStr, &tmpStr2[1]);


            if(verbose>1)
            {
                printf("CM made--- (%s) %s %d '%s'\n",CMdatabuf,querydata.toCharArray(), atoi(tmpStr), tmpStr);

            }

            allPaths[i1].setClustered(1);

            Group[CMcount-1].push_back(atoi(querydata.toCharArray()));
            //Group[CMcount-1].push_back(atoi(tmpStr));
            indexCM.store(newCM, newCMdata);

        }
        else if(querypoint.getDistance(CMpoints[0]) > radius)
        {

            newCM=new_HnPoint(querypoint);
            CMcount++;
            sprintf(CMdatabuf,"%d",CMcount-1);
            newCMdata = new_HnDataItem(CMdatabuf,10);

            char tmpStr[100] = "", tmpStr2[100];

            strcpy(tmpStr2, querydata.toCharArray());
            tmpStr2[strlen(tmpStr2-2)] = '\0';
            strcpy(tmpStr, &tmpStr2[1]);

            if(verbose>1)
            {
                printf("CM made--- (%s) %s %d '%s', distance = %f \n",CMdatabuf,querydata.toCharArray(), atoi(tmpStr), tmpStr, querypoint.getDistance(CMpoints[0]));
            }

            allPaths[i1].setClustered(1);

            Group[CMcount-1].push_back(atoi(querydata.toCharArray()));
            //Group[CMcount-1].push_back(atoi(tmpStr));
            indexCM.store(newCM, newCMdata);

        }

    }

    //    copy(CPUtimes1.begin(),CPUtimes1.end(),ostream_iterator<double>(cout,"\n"));

    for(i2=0;i2<allPaths.size();i2++)
    {
        HnPoint querypoint;
        HnDataItem querydata;
        HnDataItem newCMdata;
        int Gindex;

        HnPointVector CMpoints;
        HnDataItemVector CMdataItems;

        querypoint = allPaths.elementAt(i2);
        querydata = allData.elementAt(i2);

        if(querypoint.isClustered()==0)
        {
            indexCM.getNeighbors(querypoint,1,&CMpoints,&CMdataItems);

            if(CMpoints.size() > 0)
            {
                Gindex = atoi(CMdataItems[0].toCharArray());
                char tmpStr[100] = "", tmpStr2[100];

                strcpy(tmpStr2, querydata.toCharArray());
                tmpStr2[strlen(tmpStr2-2)] = '\0';
                strcpy(tmpStr, &tmpStr2[1]);
                //Group[Gindex].push_back(atoi(tmpStr));
                Group[Gindex].push_back(atoi(querydata.toCharArray()));
                if(verbose >1)
                {
                    printf("Clustering %s to CM(%d) \n",querydata.toCharArray(),Gindex);
                }

            }
            else
            {
                printf("What??? \n");
            }
        }

    }

    endProfiling(2);
    if(verbose)
        printf("Finished:Number of Group (%d)\n",CMcount);

    for(int i=0;i<CMcount;i++)
    {
        sort(Group[i].begin(),Group[i].end());
        if(verbose)
            copy(Group[i].begin(),Group[i].end(),ostream_iterator<int>(cout," "));
        //copy(Group[i].begin(),Group[i].end(),ostream_iterator<int>(resultFile," "));
        for(vector<int>::iterator I=Group[i].begin();I!=Group[i].end();I++)
        {
            resultFile << *I << " ";
            std::cout << *I << " ";
        }
        resultFile << endl;
        std::cout << endl;
        if(verbose)
            printf("\n");
    }
    resultFile.close();

    if(verbose)
        printf("End of clustering \n");

    /* End of clustering */


    RecordFileSt_close(recordFile);
    indexCM.close();

    freeProfileData();


    return Result;
}

    CAresultSt
ClusterAccuracy(vector<vector<int> > Clustered, int CMcount, char *congFileName,int verbose)
{
    vector<vector<int> > v1(16);
    vector<int> g1;
    int i,j,k,group,Gcount;
    int GC;
    int total;
    int SHRtotal;
    int Ocount,FalsePOS,FalseNEG;
    char dummy[32];
    FILE *fp;
    int Answer,Test;
    CAresultSt Result;

    fp=fopen(congFileName,"r");

    Gcount = 0;
    for(i=16;i<32;i++)
    {
        fscanf(fp,"%s %d\n",&dummy,&group);

        GC=0;
        count(g1.begin(),g1.end(),group,GC);

        if(GC==0)
        {
            g1.push_back(group);
            v1[Gcount].push_back(i);
            Gcount++;
        }
        else
        {
            v1[Gcount-1].push_back(i);
        }
    }
    fclose(fp);


    if(verbose)
    {
        cout << "# of groups (Answer) = " << Gcount << endl;

        for(i=0;i<Gcount;i++)
        {
            copy(v1[i].begin(),v1[i].end(),ostream_iterator<int>(cout," "));
            cout << endl;

        }
    }

    /* Probability of any error */
    Ocount=0;
    FalsePOS=0;
    FalseNEG=0;
    total=0;
    SHRtotal=0;

    for(i=16;i<31;i++)
    {
        for(j=i+1;j<32;j++)
        {
            total++;
            Answer=0;
            Test=0;

            for(k=0;k<Gcount;k++)
            {
                if(find(v1[k].begin(),v1[k].end(),i) < v1[k].end())
                {
                    if(find(v1[k].begin(),v1[k].end(),j) < v1[k].end())
                    {
                        Answer = 1;
                        SHRtotal++;
                    }
                    break;
                }
            }

            for(k=0;k<CMcount;k++)
            {
                if(find(Clustered[k].begin(),Clustered[k].end(),i) < Clustered[k].end())
                {
                    if(find(Clustered[k].begin(),Clustered[k].end(),j) < Clustered[k].end())
                    {
                        Test = 1;
                    }
                    break;
                }
            }

            //printf("(%d,%d)   Answer = %d   Test = %d \n",i,j,Answer,Test);
            if(Answer ==1 && Test == 0)
            {
                FalseNEG++;
            }
            else if(Answer == 0 && Test == 1)
            {
                FalsePOS++;
            }
            else
            {
                Ocount++;
            }
        }
    }

    if(verbose)
        printf(" Correct = %d/%d FalsePOS = %d/%d FalseNEG = %d/%d SharedTotal = %d \n",Ocount,total,FalsePOS,total-SHRtotal,FalseNEG,SHRtotal,SHRtotal);

    Result.Ocount = Ocount/(total*1.0);
    Result.FalsePOS = FalsePOS/((total-SHRtotal)*1.0);
    Result.FalseNEG = FalseNEG/(SHRtotal*1.0);

    return Result ;
}

/*
 * Profiling
 */

    static void
initializeProfileData(void)
{
    ST1 = HnTimesSt_allocate();
    ST2 = HnTimesSt_allocate();
    ET1 = HnTimesSt_allocate();
    ET2 = HnTimesSt_allocate();
    RT1 = HnTimesSt_allocate();
    RT2 = HnTimesSt_allocate();
}

    static void
startProfiling(int i)
{
    if(i==1)
        HnTimesSt_setCurrentTimes(ST1);
    else
        HnTimesSt_setCurrentTimes(ST2);
}

    static void
endProfiling(int i)
{
    if(i==1)
    {

        HnTimesSt_setCurrentTimes(ET1);
        HnTimesSt_subtract(RT1,ET1,ST1);
    }
    else
    {
        HnTimesSt_setCurrentTimes(ET2);
        HnTimesSt_subtract(RT2,ET2,ST2);
    }
}

    static void
printProfileData(int i)
{
    if(i==1)
        HnTimesSt_print(RT1);
    else
        HnTimesSt_print(RT2);
}

    static void
freeProfileData(void)
{
    HnTimesSt_free(ST1);
    HnTimesSt_free(ET1);
    HnTimesSt_free(RT1);

    HnTimesSt_free(ST2);
    HnTimesSt_free(ET2);
    HnTimesSt_free(RT2);
}
