# This file was created automatically by SWIG.
# Don't modify this file, modify the SWIG interface instead.
# This file is compatible with both classic and new-style classes.

import _hv

def _swig_setattr(self,class_type,name,value):
    if (name == "this"):
        if isinstance(value, class_type):
            self.__dict__[name] = value.this
            if hasattr(value,"thisown"): self.__dict__["thisown"] = value.thisown
            del value.thisown
            return
    method = class_type.__swig_setmethods__.get(name,None)
    if method: return method(self,value)
    self.__dict__[name] = value

def _swig_getattr(self,class_type,name):
    method = class_type.__swig_getmethods__.get(name,None)
    if method: return method(self)
    raise AttributeError,name

import types
try:
    _object = types.ObjectType
    _newclass = 1
except AttributeError:
    class _object : pass
    _newclass = 0
del types



hvMain = _hv.hvMain

hvKill = _hv.hvKill

hvReadFile = _hv.hvReadFile

getSelected = _hv.getSelected

getGraphCenter = _hv.getGraphCenter

selectCB = _hv.selectCB
class HypView(_object):
    __swig_setmethods__ = {}
    __setattr__ = lambda self, name, value: _swig_setattr(self, HypView, name, value)
    __swig_getmethods__ = {}
    __getattr__ = lambda self, name: _swig_getattr(self, HypView, name)
    def __repr__(self):
        return "<C HypView instance at %s>" % (self.this,)
    def __init__(self, *args):
        _swig_setattr(self, HypView, 'this', _hv.new_HypView(*args))
        _swig_setattr(self, HypView, 'thisown', 1)
    def afterRealize(*args): return _hv.HypView_afterRealize(*args)
    def __del__(self, destroy=_hv.delete_HypView):
        try:
            if self.thisown: destroy(self)
        except: pass
    def enumerateSubtree(*args): return _hv.HypView_enumerateSubtree(*args)
    def flashLink(*args): return _hv.HypView_flashLink(*args)
    def getCenter(*args): return _hv.HypView_getCenter(*args)
    def gotoNode(*args): return _hv.HypView_gotoNode(*args)
    def gotoCenterNode(*args): return _hv.HypView_gotoCenterNode(*args)
    def gotoPickPoint(*args): return _hv.HypView_gotoPickPoint(*args)
    def newLayout(*args): return _hv.HypView_newLayout(*args)
    def saveGraph(*args): return _hv.HypView_saveGraph(*args)
    def setCurrentCenter(*args): return _hv.HypView_setCurrentCenter(*args)
    def setGraphCenter(*args): return _hv.HypView_setGraphCenter(*args)
    def getGraphCenter(*args): return _hv.HypView_getGraphCenter(*args)
    def setGraph(*args): return _hv.HypView_setGraph(*args)
    def initGraph(*args): return _hv.HypView_initGraph(*args)
    def setDisableGroup(*args): return _hv.HypView_setDisableGroup(*args)
    def setGroupKey(*args): return _hv.HypView_setGroupKey(*args)
    def setSelected(*args): return _hv.HypView_setSelected(*args)
    def setSelectedSubtree(*args): return _hv.HypView_setSelectedSubtree(*args)
    def addLink(*args): return _hv.HypView_addLink(*args)
    def addNode(*args): return _hv.HypView_addNode(*args)
    def getChildCount(*args): return _hv.HypView_getChildCount(*args)
    def getDrawLink(*args): return _hv.HypView_getDrawLink(*args)
    def getDrawNode(*args): return _hv.HypView_getDrawNode(*args)
    def getIncomingCount(*args): return _hv.HypView_getIncomingCount(*args)
    def getOutgoingCount(*args): return _hv.HypView_getOutgoingCount(*args)
    def resetColorLink(*args): return _hv.HypView_resetColorLink(*args)
    def setLinkPolicy(*args): return _hv.HypView_setLinkPolicy(*args)
    def setColorGroup(*args): return _hv.HypView_setColorGroup(*args)
    def setColorLink(*args): return _hv.HypView_setColorLink(*args)
    def setDrawBackFrom(*args): return _hv.HypView_setDrawBackFrom(*args)
    def setDrawBackTo(*args): return _hv.HypView_setDrawBackTo(*args)
    def setDrawLink(*args): return _hv.HypView_setDrawLink(*args)
    def setDrawLinks(*args): return _hv.HypView_setDrawLinks(*args)
    def setDrawNode(*args): return _hv.HypView_setDrawNode(*args)
    def setDrawNodes(*args): return _hv.HypView_setDrawNodes(*args)
    def setNegativeHide(*args): return _hv.HypView_setNegativeHide(*args)
    def setNodeGroup(*args): return _hv.HypView_setNodeGroup(*args)
    def bindCallback(*args): return _hv.HypView_bindCallback(*args)
    def drawFrame(*args): return _hv.HypView_drawFrame(*args)
    def idle(*args): return _hv.HypView_idle(*args)
    def motion(*args): return _hv.HypView_motion(*args)
    def mouse(*args): return _hv.HypView_mouse(*args)
    def passive(*args): return _hv.HypView_passive(*args)
    def redraw(*args): return _hv.HypView_redraw(*args)
    def reshape(*args): return _hv.HypView_reshape(*args)
    def setHiliteCallback(*args): return _hv.HypView_setHiliteCallback(*args)
    def setLabelToRight(*args): return _hv.HypView_setLabelToRight(*args)
    def setPickCallback(*args): return _hv.HypView_setPickCallback(*args)
    def setFrameEndCallback(*args): return _hv.HypView_setFrameEndCallback(*args)
    def addSpanPolicy(*args): return _hv.HypView_addSpanPolicy(*args)
    def clearSpanPolicy(*args): return _hv.HypView_clearSpanPolicy(*args)
    def getDynamicFrameTime(*args): return _hv.HypView_getDynamicFrameTime(*args)
    def getIdleFrameTime(*args): return _hv.HypView_getIdleFrameTime(*args)
    def getPickFrameTime(*args): return _hv.HypView_getPickFrameTime(*args)
    def getCenterShow(*args): return _hv.HypView_getCenterShow(*args)
    def getCenterLargest(*args): return _hv.HypView_getCenterLargest(*args)
    def getEdgeSize(*args): return _hv.HypView_getEdgeSize(*args)
    def getGenerationNodeLimit(*args): return _hv.HypView_getGenerationNodeLimit(*args)
    def getGenerationLinkLimit(*args): return _hv.HypView_getGenerationLinkLimit(*args)
    def getGotoStepSize(*args): return _hv.HypView_getGotoStepSize(*args)
    def getLabels(*args): return _hv.HypView_getLabels(*args)
    def getLabelSize(*args): return _hv.HypView_getLabelSize(*args)
    def getLabelFont(*args): return _hv.HypView_getLabelFont(*args)
    def getLeafRad(*args): return _hv.HypView_getLeafRad(*args)
    def getMaxLength(*args): return _hv.HypView_getMaxLength(*args)
    def getMotionCull(*args): return _hv.HypView_getMotionCull(*args)
    def getNegativeHide(*args): return _hv.HypView_getNegativeHide(*args)
    def getPassiveCull(*args): return _hv.HypView_getPassiveCull(*args)
    def getSphere(*args): return _hv.HypView_getSphere(*args)
    def getSpanPolicy(*args): return _hv.HypView_getSpanPolicy(*args)
    def getTossEvents(*args): return _hv.HypView_getTossEvents(*args)
    def setCenterLargest(*args): return _hv.HypView_setCenterLargest(*args)
    def setCenterShow(*args): return _hv.HypView_setCenterShow(*args)
    def setEdgeSize(*args): return _hv.HypView_setEdgeSize(*args)
    def setGenerationNodeLimit(*args): return _hv.HypView_setGenerationNodeLimit(*args)
    def setGenerationLinkLimit(*args): return _hv.HypView_setGenerationLinkLimit(*args)
    def setGotoStepSize(*args): return _hv.HypView_setGotoStepSize(*args)
    def setKeepAspect(*args): return _hv.HypView_setKeepAspect(*args)
    def setLabels(*args): return _hv.HypView_setLabels(*args)
    def setLabelSize(*args): return _hv.HypView_setLabelSize(*args)
    def setLabelFont(*args): return _hv.HypView_setLabelFont(*args)
    def setLeafRad(*args): return _hv.HypView_setLeafRad(*args)
    def setMaxLength(*args): return _hv.HypView_setMaxLength(*args)
    def setMotionCull(*args): return _hv.HypView_setMotionCull(*args)
    def setPassiveCull(*args): return _hv.HypView_setPassiveCull(*args)
    def setSphere(*args): return _hv.HypView_setSphere(*args)
    def setTossEvents(*args): return _hv.HypView_setTossEvents(*args)
    def setDynamicFrameTime(*args): return _hv.HypView_setDynamicFrameTime(*args)
    def setIdleFrameTime(*args): return _hv.HypView_setIdleFrameTime(*args)
    def setPickFrameTime(*args): return _hv.HypView_setPickFrameTime(*args)
    def setColorBackground(*args): return _hv.HypView_setColorBackground(*args)
    def setColorHilite(*args): return _hv.HypView_setColorHilite(*args)
    def setColorLabel(*args): return _hv.HypView_setColorLabel(*args)
    def setColorLinkFrom(*args): return _hv.HypView_setColorLinkFrom(*args)
    def setColorLinkTo(*args): return _hv.HypView_setColorLinkTo(*args)
    def setColorSelect(*args): return _hv.HypView_setColorSelect(*args)
    def setColorSphere(*args): return _hv.HypView_setColorSphere(*args)
    def getHypGraph(*args): return _hv.HypView_getHypGraph(*args)

class HypViewPtr(HypView):
    def __init__(self, this):
        _swig_setattr(self, HypView, 'this', this)
        if not hasattr(self,"thisown"): _swig_setattr(self, HypView, 'thisown', 0)
        _swig_setattr(self, HypView,self.__class__,HypView)
_hv.HypView_swigregister(HypViewPtr)


