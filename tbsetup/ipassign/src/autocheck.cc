// autocheck.cpp
//
// Copyright (c) 2003 Jonathon Duerig.
// Usage is according to the BSD license (see license.txt).

#include <cstring>
#include <cstdlib>
#include <ctime>

#include <iostream>
#include <iomanip>
#include <string>
#include <sstream>
#include <fstream>

using namespace std;

const char SLASH ='/';
const char ANTI_SLASH = '\\';

namespace arg
{
    bool printOutput;
    bool timeTests;
    bool zeroIndex;
    string configFileName;

    string trialOutputPath;
    string scriptPath;
    string keyOutputPath;
    string runCommand;
    string compareCommand;
}

bool processArgumentList(int argc, char* argv[]);
bool processArgument(char* argument);
bool processFlagList(char* argument);
void usage(void);
void version(void);

int readConfig(void);
void divideLine(void);

void variableLine(istringstream& line, const string& var, const string& value);
void setOption(const string& var, const string& value);
void makeDirectory(string& path);
void slashify(string& path);

int testLine(const string& name, const string& number);
int test(const string& name, const string& number);
void processFile(const string& name, const string& number);
int compareFile(const string& name, const string& number);
void printOutputFile(const string& name, const string& number);


int main(int argc, char* argv[])
{
    if (processArgumentList(argc, argv))
    {
        int numFails = readConfig(); // 0 means pass

        if (numFails == 0)
        {
            cout << endl << "All tests have passed." << endl;
            return 0;
        }
        else if (numFails == 1)
        {
            cout << endl << "One test has failed." << endl;
            return 1;
        }
        else
        {
            cout << endl << numFails << " tests have failed." << endl;
            return 1;
        }
    }
    else
    {
        return 0;
    }
}

//-----------------------------------------------------------------------------

// This function has multiple return points
bool processArgumentList(int argc, char* argv[])
{
    // set the defaults argument values
    arg::printOutput = false;
    arg::timeTests = false;
    arg::zeroIndex = false;
    arg::configFileName = string("etc") + SLASH + "autocheck.conf";
    arg::trialOutputPath=string(".") + SLASH;
    arg::scriptPath=string(".") + SLASH;
    arg::keyOutputPath=string(".") + SLASH;

    // change the argument values as the user requests
    for (int i = 1; i < argc; ++i)
    {
        if (!processArgument(argv[i]))
        {
            return false;
        }
    }
    return true;
}

bool processArgument(char* argument)
{
    if (argument[0] == '-')
    {
        switch (argument[1])
        {
        case '-':
            if (strncmp (argument, "--config-file=", sizeof("--config-file=") - 1) == 0)
            {
                arg::configFileName = argument + sizeof ("--config-file=") - 1;
                return true;
            }
            else
            {
                usage();
                return false;
            }
        case '\0':
            usage();
            return false;
        default:
            return processFlagList(argument);
        }
    }
    else
    {
        usage();
        return false;
    }
}

bool processFlagList(char* argument)
{
    for (int j = 1; argument[j] != '\0'; ++j)
    {
        switch (argument[j])
        {
        case 'o':
            arg::printOutput = true;
            break;
        case 'V':
            version();
            return false;
        case 't':
            arg::timeTests = true;
            break;
        case 'z':
            arg::zeroIndex = true;
            break;
        case 'h':
        case '?':
        default:
            usage();
            return false;
        }
    }
    return true;
}

void usage(void)
{
    cout << endl << endl;
    cout << "Usage: autocheck [-h] [-?] [-o] [-t] [-V] [--config-file=filename]" << endl << endl;
    cout << "The basic concept:" << endl;
    cout << "    autocheck searches through the config-file for scripts to test. For each" << endl;
    cout << "script in the list-file, it compiles it using the log-path, compile-command," << endl;
    cout << "outputting the log file for the compile to the and the compiled program to the" << endl;
    cout << "trial-program-path. Then the program is executed with the run-command. The" << endl;
    cout << "runtime log file is sent to the log-path and the output of the program" << endl;
    cout << "is captured and stored in a file in the trial-output-path. Finally, the" << endl;
    cout << "compare-command is used to compare the output and program files in the" << endl;
    cout << "trial-output/program-paths and their corresponding ones in the" << endl;
    cout << "key-output/program-paths. If the trial versions match the exemplar versions," << endl;
    cout << "then the two tests pass. And otherwise one or both tests fail. The" << endl;
    cout << "results of the tests are displayed and then the program moves to the next" << endl;
    cout << "script." << endl << endl;
    cout << "-h -?              -> Display help" << endl << endl;
    cout << "-o                 -> Show the output files on the screen while testing" << endl << endl;
    cout << "-t                 -> Time the compilation and execution of the scripts" << endl << endl;
    cout << "-V                 -> Show version information" << endl << endl;
    cout << "-z                 -> Start counting script numbers at 0 rather than 1" << endl << endl;
    cout << "--config-file=file -> The file is used to configure autocheck. It contains info" << endl;
    cout << "                      about which paths to use and what scripts to check";
    cout << "                      Default='../autocheck.conf'" << endl << endl;
}

void version(void)
{
    cout << endl << endl;
    cout << "autocheck version 1.2 (for ipassign)" << endl;
}

//-----------------------------------------------------------------------------

int readConfig(void)
{
    string buffer;
    istringstream line;

    int numFails=0;
    string name;
    string number;

    ifstream file(arg::configFileName.c_str(), ios::in);
    if (!file)
    {
        cout << endl << "Error opening the configuration file. '" << arg::configFileName.c_str() << "'." << endl;
        return 1;
    }

    getline (file, buffer);
    while (file)
    {
        line.clear();
        line.str(buffer);
        line >> name;
        if (!line)
        {
            name = "#";
        }
        line >> number;
        if (!line)
        {
            number = "";
        }
        switch(name[0])
        {
        case '=': // Print out a horizontal line to divide output
            divideLine();
            break;
        case '$': // Set an option to a particular value
            variableLine(line, name, number);
            break;
        case '#': // Comment
            break;
        default: // Description of a test to run
            numFails += testLine(name, number);
            break;
        }
        getline (file, buffer);
    }

    if (arg::timeTests)
    {
        cout << endl << "There are " << CLK_TCK << " ticks in a second." << endl;
    }

    return numFails;
}

void divideLine(void)
{
    cout << endl << "==================================" << endl;
}

//-----------------------------------------------------------------------------

void variableLine(istringstream& line, const string& var, const string& value)
{
    // value should include the rest of the line except for trailing whitespace.
    string localValue;
    getline(line, localValue);
    localValue = value + localValue;
    size_t index = localValue.size() - 1;
    while (localValue[index] == '\t' || localValue[index] == '\r'
        || localValue[index] == '\n' || localValue[index] == ' ')
    {
        localValue.resize(index);
        --index;
    }
    setOption(var, localValue);
}

void setOption(const string& var, const string& value)
{
      // Determine which variable will be replaced by number
      if (var == "$trialOutputPath")
      {
          arg::trialOutputPath = value;
          slashify(arg::trialOutputPath);
          makeDirectory(arg::trialOutputPath);
      }
      else if (var == "$scriptPath")
      {
          arg::scriptPath = value;
          slashify(arg::scriptPath);
          makeDirectory(arg::scriptPath);
      }
      else if (var == "$keyOutputPath")
      {
          arg::keyOutputPath = value;
          slashify(arg::keyOutputPath);
          makeDirectory(arg::keyOutputPath);
      }
      else if (var == "$runCommand")
      {
          arg::runCommand = value;
          slashify(arg::runCommand);
      }
      else if (var == "$compareCommand")
      {
          arg::compareCommand = value;
          slashify(arg::compareCommand);
      }
      else
      {
          cerr << endl << "Unknown variable name '" << var << "'." << endl;
      }
}

void makeDirectory(string& path)
{
    size_t index = path.size() - 1;
    if (index >= 0)
    {
        if (path[index] != SLASH)
        {
            path += SLASH;
        }
    }
}

void slashify(string& path)
{
    size_t limit=path.size();
    for (size_t i = 0; i < limit; ++i)
    {
        if (path[i] == ANTI_SLASH)
        {
            path[i] = SLASH;
        }
    }
}

//-----------------------------------------------------------------------------

int testLine(const string& name, const string& number)
{
    int numFails = 0;
    ostringstream int2string;
    istringstream string2int;
    int limit;
    string2int.str(number);
    string2int >> limit;
    if (!string2int)
    {
        limit = 1;
        string2int.clear();
    }
    int i = 1;
    if (arg::zeroIndex)
    {
        --i;
        --limit;
    }
    for ( ; i <= limit; ++i)
    {
        int2string.str("");
        int2string << i;
        numFails += test(name, int2string.str());
    }
    return numFails;
}

int test(const string& name, const string& number)
{
    int testsFailed = 0;

    cout << endl << "\tTesting " << name << " #" << number << " ";

    processFile(name, number);

    testsFailed = compareFile(name, number);

    if (arg::printOutput)
    {
        printOutputFile(name, number);
    }

    return testsFailed;
}

void processFile(const string& name, const string& number)
{
    clock_t timeStart = 0;
    clock_t timeFinish = 0;
    clock_t timeCompile = 0;
    clock_t timeRun = 0;

//    cout << "\tErrors: " << flush;

    // Run the program
    if (arg::timeTests)
    {
        timeStart = clock();
    }

    system((arg::runCommand + " < " + arg::scriptPath + name + number
            + ".graph > " + arg::trialOutputPath + name + number
            + ".result").c_str());

    if (arg::timeTests)
    {
        timeFinish = clock();
        timeRun = timeFinish - timeStart;
        cout << endl << "Run time: " << timeRun << " ticks";
    }
}

int compareFile(const string& name, const string& number)
{
    int testsFailed = 0;
    int currentTest = 0;

    // test the output file
    cout << ": ";
    currentTest = system(( arg::compareCommand + ' ' + arg::keyOutputPath
                           + name + number + ".result " + arg::trialOutputPath
                           + name + number + ".result").c_str());
    if (currentTest == 0)
        cout << "-->  pass " << endl;
    else
    {
        cout << "--> -FAIL-" << endl;
        ++testsFailed;
    }
    return testsFailed;
}

void printOutputFile(const string& name, const string& number)
{
    ifstream file((arg::trialOutputPath + name + number + "-output.txt").c_str(), ios::in);
    if (!file)
    {
        cout << "Error opening output file '" << arg::trialOutputPath << name << number;
        cout << ".result' for display." << endl;
    }
    else
    {
        string buffer;
        getline(file, buffer);
        cout << endl << "---- Begin output file ----" << endl;
        while (file)
        {
            cout << buffer << endl;
            getline(file, buffer);
        }
        cout << "---- End output file ------" << endl;
    }
}





