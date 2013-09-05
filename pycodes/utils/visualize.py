"""

     Copyright 2003 Mitsubishi Electric Research Laboratories All Rights
     Reserved.  Permission to use, copy and modify this software and its
     documentation without fee for educational, research and non-profit
     purposes, is hereby granted, provided that the above copyright
     notice and the following three paragraphs appear in all copies.      

     To request permission to incorporate this software into commercial
     products contact:  Vice President of Marketing and Business         
     Development;  Mitsubishi Electric Research Laboratories (MERL), 201
     Broadway, Cambridge, MA   02139 or <license@merl.com>.        
                                                                               
     IN NO EVENT SHALL MERL BE LIABLE TO ANY PARTY FOR DIRECT, INDIRECT, 
     SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES, INCLUDING LOST        
     PROFITS, ARISING OUT OF THE USE OF THIS SOFTWARE AND ITS 
     DOCUMENTATION, EVEN IF MERL HAS BEEN ADVISED OF THE POSSIBILITY OF
     SUCH DAMAGES.

     MERL SPECIFICALLY DISCLAIMS ANY WARRANTIES, INCLUDING, BUT NOT
     LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
     FOR A PARTICULAR PURPOSE.  THE SOFTWARE PROVIDED HEREUNDER IS ON AN
     ``AS IS'' BASIS, AND MERL HAS NO OBLIGATIONS TO PROVIDE MAINTENANCE,
     SUPPORT, UPDATES, ENHANCEMENTS OR MODIFICATIONS.
"""

import Tkinter
from Tkconstants import *

"""
The visualize package contains routines to draw a low density parity
check code graph or the dual of a low density parity check code graph.
The main functions to call are:

VisualizeCodeGraph
VisualizeDualCodeGraph

For example,

>>> VisualizeCodeGraph(7,4,[[0,1,2,4],[1,2,3,5],[2,3,4,6]])

will display the graph for a Hamming code, while

>>> VisualizeDualCodeGraph(7,4,[[0,1,2,4],[1,2,3,5],[2,3,4,6]])

will display the graph for its dual.
"""

def MakeHighlighter(canvas):
    """
    MakeHighlighter(canvas):
    
    This function sets a callback for canvas.

    Whenever you click on a var/check that node, its edges,
    and its neighbors will be highlighted.

    Whenever you click on an edge, that edge and the
    connected nodes will be highlighted
    
    If you clicked with the left button, anything that was
    previously highlighted is unhighlighted.

    If you clicked with the right button, anything that was
    previously highlighted is still highlighted.
    """
    def DoHighlight(event):
        X = event.x + canvas.xview()[0]*canvas.scrollsize
        Y = event.y + canvas.yview()[0]*canvas.scrollsize
        node = canvas.find_overlapping(X-5,Y-5,X+5,Y+5)
        if (event.num == 1):
            canvas.addtag_withtag('now','edge')
            canvas.itemconfig('now',fill='black')
            canvas.dtag('now')
            canvas.addtag_withtag('now','var')
            canvas.addtag_withtag('now','check')        
            canvas.itemconfig('now',outline='black',fill='')
            canvas.dtag('now')        
            canvas.dtag('connected')
        for tagName in canvas.gettags(node):
            if (tagName[0:2] == 'c_' or tagName[0:2] == 'v_'):
                canvas.addtag_withtag('connected','e_' + tagName)
                for item in canvas.find_withtag('connected'):
                    for t in canvas.itemcget(item,'tags').split():
                        if (t[0:2] == 'e_'):
                            canvas.addtag_withtag('connected',t[2:])

            elif (tagName[0:2] == 'e_'):
                canvas.addtag_withtag('connected',tagName[2:])
                canvas.itemconfig(node,fill='blue')
        canvas.itemconfig('connected',fill='blue')
                
    canvas.bind('<Button>',DoHighlight)

def CreateScrollableCanvas(w,canvasSize=500,width=700,height=700):
    """
    CreateScrollableCanvas(w,canvasSize=500,width=700,height=700):

    w:           Window to pack the canvas into.
    canvasSize:  Total area of the canvas.  Note that if width
                 and/or height exceeds canvasSize you will need
                 to scroll to see the rest of the canvas.
    width:       Width to make the canvas.
    height:       Width to make the canvas.

    Creates a canvas object with scroll bars, packs it in w and
    returns the newly created canvas object.
    """
    w.yscroll=Tkinter.Scrollbar(w,orient=VERTICAL)
    w.xscroll=Tkinter.Scrollbar(w,orient=HORIZONTAL)
    w.canvas = Tkinter.Canvas(w,scrollregion=(0,0,canvasSize,canvasSize),
                            xscrollcommand=w.xscroll.set,
                            yscrollcommand=w.yscroll.set,
                            width=width,height=height)
    w.canvas.scrollsize = canvasSize
    w.yscroll.config(command=w.canvas.yview)
    w.xscroll.config(command=w.canvas.xview)
    w.yscroll.pack(side=RIGHT,fill=Y,expand=0)
    w.xscroll.pack(side=BOTTOM,fill=X,expand=0)
    w.canvas.pack(fill=BOTH,expand=0)
    return w.canvas

def CreateVarNode(canvas,idNumber,x,y):
    """
    CreateVarNode(canvas,idNumber,x,y):

    Creates a variable node at the given x,y coordinate in the canvas
    and give it the tag 'v_'+`idNumber`.
    """
    bboxHalfWidth = 40
    o=canvas.create_oval((x-bboxHalfWidth,y-bboxHalfWidth,
                          x+bboxHalfWidth,y+bboxHalfWidth),
                         tag='v_'+`idNumber` + ' var')
    canvas.create_text((x,y),text=`idNumber`)
    
def CreateCheckNode(canvas,idNumber,x,y):
    """
    CreateCheckNode(canvas,idNumber,x,y):

    Creates a check node at the given x,y coordinate in the canvas
    and give it the tag 'c_'+`idNumber`.
    """
    bboxHalfWidth = 40
    canvas.create_rectangle((x-bboxHalfWidth,y-bboxHalfWidth,
                             x+bboxHalfWidth,y+bboxHalfWidth),
                            tag='c_'+`idNumber` + ' check')
    canvas.create_line(x-bboxHalfWidth/2.0,y,x+bboxHalfWidth/2.0,y)
    canvas.create_line(x,y-bboxHalfWidth/2.0,x,y+bboxHalfWidth/2.0)    

def LinkVarToChecks(canvas,varID,checkID,varMax=4,checkMax=4):
    """
    LinkVarToChecks(canvas,varID,checkID,varMax=4,checkMax=4):

    Creates a link linking the var with tag 'v_'+`varID` to the check
    with tag 'c_'+`checkID`.  An attempt is made to make the outgoing
    edges emanate from different points on the var/check by counting
    the number of existing edges for the given var/check.  This
    attempt will work best if you can give the maximum number of
    edges that will connect to the var/check.
    """
    vPos = canvas.coords('v_'+`varID`)
    cPos = canvas.coords('c_'+`checkID`)
    checkTag = 'e_c_'+`checkID`
    varTag = 'e_v_'+`varID`
    checkCount = len(canvas.find_withtag(checkTag))
    varCount = len(canvas.find_withtag(varTag))
    varMax = max(varMax,varCount+1)
    checkMax = max(checkMax,checkCount+1)
    vWidth = (vPos[2]-vPos[0])*((1+varCount)/float(varMax+1))
    cWidth = (cPos[2]-cPos[0])*((1+checkCount)/float(checkMax+1))
    
    if (vPos[1] > cPos[1]): # var is below check
        lineCoords = (vPos[0]+vWidth , vPos[1],
                      cPos[0]+cWidth, cPos[3] )
    else: # var is above check
        lineCoords = (vPos[0]+vWidth , vPos[3],
                      cPos[0]+cWidth, cPos[1] )
    
    line = canvas.create_line(lineCoords,tag=varTag+' '+checkTag+' edge')

def VisualizeDualCodeGraph(N,K,L,w=None):
    """
    VisualizeDualCodeGraph(N,K,L,w=None):

    N:       Block length of normal code.
    K:       Dimension of normal code.
    L:       Link array for the normal code.  Specifically, L[i][j]==1
             if and only if check i is connected to variable j.  Note
             that counting starts with check 0 and var 0.
    w:       Window to create a canvas for the graph in.

    Creates the dual graph of the code with the given paramters in w.
    If w==None, then the root window is used.

    If you click on a node then that node and all its edges and
    neighbors will be highlighted.  Left clicks unhighlight everything
    else while right clicks preserve highlighting.
    """
    dualK = N-K # the dimension of the dual code is N-dimension of normal code
    if (w==None):
        w = Tkinter.Tk()

    horzOffset = 50
    obsVarHeight = 50
    checkHeight = 150
    checkHorzInc = 100
    checkWidth = checkHorzInc*N
    varHorzInc = checkHorzInc*N/float(dualK)
    hidVarHeight = 600    

    canvas = CreateScrollableCanvas(w,canvasSize=checkWidth+2*horzOffset)
    
    for i in range(N):
        CreateVarNode(canvas,i+dualK,horzOffset+i*checkHorzInc,obsVarHeight)
        CreateCheckNode(canvas,i,horzOffset+i*checkHorzInc,checkHeight)
        LinkVarToChecks(canvas,i+dualK,i,1,1)
    for i in range(dualK):
        CreateVarNode(canvas,i,horzOffset+i*varHorzInc,hidVarHeight)

    for rowIndex in range(len(L)):
        for item in L[rowIndex]:
            LinkVarToChecks(canvas,rowIndex,item,varMax=len(L[rowIndex]))
    MakeHighlighter(canvas)
    return w

def VisualizeCodeGraph(N,K,L,w=None):
    """
    VisualizeCodeGraph(N,K,L,w=None):

    N:       Block length of code.
    K:       Dimension of code.
    L:       Link array for the code.  Specifically, L[i][j]==1
             if and only if check i is connected to variable j.  Note
             that counting starts with check 0 and var 0.    
    w:       Window to create a canvas for the graph in.

    Creates the graph of the code with the given paramters in w.
    If w==None, then the root window is used.

    If you click on a node then that node and all its edges and
    neighbors will be highlighted.  Left clicks unhighlight everything
    else while right clicks preserve highlighting.
    """
    
    if (w==None):
        w = Tkinter.Tk()
    
    horzOffset = 50
    varHeight = 50
    checkHeight = 150
    varHorzInc = 100
    varWidth = varHorzInc*N
    checkWidth = varWidth*float(N-K)/float(K)
    checkHorzInc = varWidth/float(N-K)
    checkHeight = 600    

    canvas = CreateScrollableCanvas(w,canvasSize=varWidth+2*horzOffset)
    
    for i in range(N):
        CreateVarNode(canvas,i,horzOffset+i*varHorzInc,varHeight)
    for rowIndex in range(len(L)):
        CreateCheckNode(canvas,rowIndex,
                        horzOffset+rowIndex*checkHorzInc,checkHeight)
        for item in L[rowIndex]:
            LinkVarToChecks(canvas,item,rowIndex,checkMax=len(L[rowIndex]))
    MakeHighlighter(canvas)        
    return w



