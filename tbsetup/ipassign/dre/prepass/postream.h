// This was taken from the below book for speed-of-implementation purposes.

/* The following code example is taken from the book
 * "The C++ Standard Library - A Tutorial and Reference"
 * by Nicolai M. Josuttis, Addison-Wesley, 1999
 *
 * (C) Copyright Nicolai M. Josuttis 1999.
 * Permission to copy, use, modify, sell and distribute this software
 * is granted provided this copyright notice appears in all copies.
 * This software is provided "as is" without express or implied
 * warranty, and with no claim as to its suitability for any purpose.
 */
#include <iostream>
#include <streambuf.h>
#include <cstdio>
#include <unistd.h>

class poutbuf : public std::streambuf {
private:
    poutbuf();
  protected:
    int file;    // file descriptor
  public:
    // constructor
    poutbuf (int newfile) : file(newfile) {
    }
  protected:
    // write one character
    virtual int overflow (int c) {
        if (c != EOF) {
            char z = c;
            if (write (file, &z, sizeof(char)) != 1) {
                return EOF;
            }
        }
        return c;
    }
    // write multiple characters
    virtual
    std::streamsize xsputn (const char* s,
                            std::streamsize num) {
        return write(file, s, num);
    }
};

class postream : public std::ostream {
protected:
    poutbuf buf;
public:
    postream (int file) : std::ostream(0), buf(file) {
        rdbuf(&buf);
    }
private:
    postream();
};
