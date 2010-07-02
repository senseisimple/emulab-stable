# aspects.py - Tools for AOP in Python
# Version 0.2
#
# Copyright (C) 2003-2005 Antti Kervinen (ask@cs.tut.fi)
#
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License as published by the Free Software Foundation; either
# version 2.1 of the License, or (at your option) any later version.
#
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public
# License along with this library; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

import re

def wrap_around_re (aClass, wildcard, advice):
    """ Same as wrap_around but works with regular expression based
    wildcards to map which methods are going to be used. """
    
    matcher = re.compile (wildcard)

    for aMember in dir (aClass):
        realMember = getattr (aClass, aMember)
        if callable (realMember) and aMember[:6]!="__wrap" and \
           aMember[:9]!="__proceed" and \
           matcher.match (aMember):
            wrap_around (realMember, advice)
            
def wrap_around( method, advice ):
    """ wrap_around wraps an unbound method (the first argument)
    inside an advice (function given as the second argument). When the
    method is called, the code of the advice is executed to the point
    of "self.__proceed(*args,**keyw)". Then the original method is
    executed after which the control returns to the advice.  """

    methods_name = method.__name__
    methods_class = method.im_class

    if methods_name[:2]=="__" and not hasattr(methods_class,methods_name):
        methods_name="_"+methods_class.__name__+methods_name

    # Check if __proceed method is implemented in the class
    try:
        getattr( methods_class,'__proceed')
    except:
        setattr( methods_class, '__proceed_stack', [] )
        setattr( methods_class,'__proceed', _proceed_method )

    # Check how many times this method has been wrapped
    try:
        methodwrapcount = getattr( methods_class, '__wrapcount'+methods_name )
    except:
        methodwrapcount = 0
        setattr( methods_class, '__wrapcount'+methods_name, methodwrapcount )
    
    # Rename the original method: it will be __wrapped/n/origmethodname
    # where /n/ is the number of wraps around it
    wrapped_method_name = "__wrapped" + str(methodwrapcount) + methods_name
    orig_method = getattr( methods_class, methods_name )
    setattr( methods_class, wrapped_method_name, orig_method )
    
    # Add the wrap (that is, the advice) as a new method
    wrapper_method_name = "__wrap" + str(methodwrapcount) + methods_name
    setattr( methods_class, wrapper_method_name, advice )
    
    # Replace the original method by a method that
    # 1) sets which method should be executed in the next proceed call
    # 2) calls the wrap (advice)
    new_code = "def " + methods_name + "(self,*args,**keyw):\n" + \
               "\tself.__proceed_stack.append( _Proceed_stack_entry(self." + wrapped_method_name + ",'" + methods_class.__name__ + "." + methods_name + "'))\n" + \
               "\ttry: retval = self." + wrapper_method_name + "(*args,**keyw)\n" + \
               "\tfinally: self.__proceed_stack.pop()\n" + \
               "\treturn retval\n"
    new_method = _create_function( new_code, methods_name )
    setattr( methods_class, methods_name, new_method )

    setattr( methods_class, '__wrapcount'+methods_name, methodwrapcount+1 )


def peel_around( method ):
    """ Removes one wrap around the method (given as a parameter) and
    returns the wrap. If the method is not wrapped, returns None."""

    methods_name = method.__name__
    methods_class = method.im_class

    try:
        wc = getattr( methods_class, '__wrapcount'+methods_name ) - 1
    except:
        return None
    
    if wc==-1: return None
    
    wrapped = getattr( methods_class, '__wrapped' + str(wc) + methods_name )
    setattr( methods_class, methods_name, wrapped )
    setattr( methods_class, '__wrapcount'+methods_name, wc )

    removed_adv = getattr( methods_class, '__wrap'+str(wc)+methods_name )
    del methods_class.__dict__['__wrapped'+str(wc)+methods_name]
    del methods_class.__dict__['__wrap'+str(wc)+methods_name]
    
    return removed_adv

def _proceed_method( self, *args, **keyw ):
    method = self.__proceed_stack[-1].method
    return method( *args, **keyw )


def _create_function( function_code, function_name ):
    codeobj = compile( function_code, "", "exec" )
    exec( codeobj )
    return eval( function_name )

class _Proceed_stack_entry:
    def __init__(self, method, name):
        self.method = method
        self.name = name

### Module test

if __name__=="__main__":

    class C:
        def __foo0(self,num): return num
        def foo1(self,num): return self.__foo0(num)+1        
        def foo2(self,num): return self.__foo0(num)+2
        def bar1(self,num): return self.__foo0(num)+4

    def adv1(self,*args):
        return self.__proceed(args[0]+100)

    def adv2(self,*args,**keyw):
        return self.__proceed(keyw['num']+400)

    o=C()

    test="wrap_around a public method, keyword argument"
    verdict="FAIL"
    try:
        wrap_around(C.foo1,adv2)
        if o.foo1(num=0)==401:
            verdict="PASS"
    finally:
        print verdict,test

    test="wrap_around a private method (called by the public method)"
    verdict="FAIL"
    try:
        wrap_around(C._C__foo0,adv1)
        if o.foo1(num=0)==501:
            verdict="PASS"
    finally:
        print verdict,test

    test="peel_around clear advices"
    verdict="FAIL"
    try:
        peel_around(C.foo1)
        peel_around(C._C__foo0)
        if o.foo1(num=0)==1:
            verdict="PASS"
    finally:
        print verdict,test
    
    test="wrap_around_re, wrap_around, normal arguments"
    verdict="FAIL"
    try:
        wrap_around_re(C,'foo[0-9]',adv1)
        if o.foo1(0)==101 and o.foo2(0)==102:
            verdict="PASS"
    finally:
        print verdict,test

    test="peel_around 1/2: remove advice"
    verdict="FAIL"
    try:
        peel=peel_around(C.foo1)
        if o.foo1(0)==1:
            verdict="PASS"
    finally:
        print verdict,test

    test="peel_around 2/2: rewrap peeled method. One method wrapped twice."
    verdict="FAIL"
    try:
        wrap_around_re(C,'.*',peel)
        if o.foo1(0)==201 and o.foo2(0)==302 and o.bar1(0)==204:
            verdict="PASS"
    finally:
        print verdict,test

    test="keyword arguments, two different wraps on one method"
    verdict="FAIL"
    try:
        wrap_around(C.bar1,adv2)
        if o.bar1(num=8)==612:
            verdict="PASS"
    finally:
        print verdict,test

