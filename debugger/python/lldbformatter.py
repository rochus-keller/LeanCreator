"""

Copyright (c) 2023 Rochus Keller
Copyright (c) 2021 Pavol Markovic
Copyright (c) 2015 Luke Worth, Sean Donegan

** This file is part of LeanCreator.
**
** $QT_BEGIN_LICENSE:LGPL21$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.

"""

import lldb

class qlist_provider:
    def __init__(self, valobj, dict):
        self.v = valobj
        self.d = self.v.GetChildMemberWithName('p').GetChildMemberWithName('d')
        self.array = self.d.GetChildMemberWithName('array')
        self.list_type = self.v.GetType().GetTemplateArgumentType(0)
        #print("list_type:", self.list_type.name)
        #self.element_size = self.list_type.GetByteSize()
        self.ptr_size = self.v.GetProcess().GetAddressByteSize()

    def num_children(self):
        end = self.d.GetChildMemberWithName('end').GetValueAsUnsigned(0)
        begin = self.d.GetChildMemberWithName('begin').GetValueAsUnsigned(0)
        return end - begin

    def get_child_index(self, name):
        n = -1
        try:
            n = int(name.lstrip('[').rstrip(']'))
        except:
            print("get_child_index exception")
        #print("get_child_index",name,n)
        return n

    def get_child_at_index(self, index):
        child = None
        try:
            pBegin = self.d.GetChildMemberWithName('begin').GetValueAsUnsigned()
            return self.array.CreateChildAtOffset('[' + str(index) + ']', pBegin + index * self.ptr_size, self.list_type)
            """
            child_data = self.array.GetPointeeData(index)
            val = self.array.CreateValueFromData('dummy', child_data, self.array.GetType())
            child_at_zero = val.GetChildAtIndex(0)
            if self.element_size > self.ptr_size:
                child = child_at_zero.CreateChildAtOffset('[' + str(index) + ']', 0, self.list_type)
            else:
                child = child_at_zero.Cast(self.list_type)
            """
        except:
            print("get_child_at_index exception")
        return child

"""
    def extract_type(self):
        t = self.v.GetType().GetUnqualifiedType()
        if t.IsReferenceType():
            t = t.GetDereferencedType()
        elif t.TypeIsPointerType():
            t = t.GetPointeeType()
        list_type = t.GetTemplateArgumentType(0)
        return list_type
        """

def qlist_summary(valobj, dict):
    return 'size=' + str(valobj.GetNumChildren())

"""        
def qlist_summary(v, dict):
    try:
        d = v.GetChildMemberWithName('d') 
        end = d.GetChildMemberWithName('end').GetValueAsUnsigned()
        begin = d.GetChildMemberWithName('begin').GetValueAsUnsigned()
        size = end - begin
        data = "size=0"
        if size > 0:
            data = "size="+format(size,"d")
        return data
    except:
        print("exception in qlist_summary")
        return "<invalid>"
"""

class qvector_provider:
    def __init__(self, valobj, internal_dict):
            self.valobj = valobj

    def num_children(self):
            try:
                    s = self.valobj.GetChildMemberWithName('d').GetChildMemberWithName('size').GetValueAsUnsigned()
                    return s
            except:
                    return 0

    def get_child_index(self,name):
            try:
                    return int(name.lstrip('[').rstrip(']'))
            except:
                    return None

    def get_child_at_index(self,index):
            if index < 0:
                    return None
            if index >= self.num_children():
                    return None
            if self.valobj.IsValid() == False:
                    return None
            try:
                    doffset = self.valobj.GetChildMemberWithName('d').GetChildMemberWithName('offset').GetValueAsUnsigned()
                    type = self.valobj.GetType().GetTemplateArgumentType(0)
                    elementSize = type.GetByteSize()
                    return self.valobj.GetChildMemberWithName('d').CreateChildAtOffset('[' + str(index) + ']', doffset + index * elementSize, type)
            except:
                    return None

def qvector_summary(valobj, dict):
    return 'size=' + str(valobj.GetNumChildren())

class qmap_provider:
    # TODO: this doesn't work yet
    def __init__(self, valobj, dict):
        self.valobj = valobj
        self.garbage = False
        self.root_node = None
        self.header = None
        self.data_type = None

    def num_children(self):
        return self.valobj.GetChildMemberWithName('d').GetChildMemberWithName('size').GetValueAsUnsigned(0)

    def get_child_index(self, name):
        try:
            return int(name.lstrip('[').rstrip(']'))
        except:
            return -1

    def get_child_at_index(self, index):
        if index < 0:
            return None
        if index >= self.num_children():
            return None
        if self.garbage:
            return None
        try:
            offset = index
            current = self.header
            while offset > 0:
                current = self.increment_node(current)
                offset -= 1
            child_data = current.Dereference().Cast(self.data_type).GetData()
            return current.CreateValueFromData('[' + str(index) + ']', child_data, self.data_type)
        except:
            return None

    def extract_type(self):
        map_type = self.valobj.GetType().GetUnqualifiedType()
        target = self.valobj.GetTarget()
        if map_type.IsReferenceType():
            map_type = map_type.GetDereferencedType()
        if map_type.GetNumberOfTemplateArguments() > 0:
            first_type = map_type.GetTemplateArgumentType(0)
            second_type = map_type.GetTemplateArgumentType(1)
            close_bracket = '>'
            if second_type.GetNumberOfTemplateArguments() > 0:
                close_bracket = ' >'
            data_type = target.FindFirstType(
                'QMapNode<' + first_type.GetName() + ', ' + second_type.GetName() + close_bracket)
        else:
            data_type = None
        return data_type

    def node_ptr_value(self, node):
        return node.GetValueAsUnsigned(0)

    def right(self, node):
        return node.GetChildMemberWithName('right')

    def left(self, node):
        return node.GetChildMemberWithName('left')

    def parent(self, node):
        parent = node.GetChildMemberWithName('p')
        parent_val = parent.GetValueAsUnsigned(0)
        parent_mask = parent_val & ~3
        parent_data = lldb.SBData.CreateDataFromInt(parent_mask)
        return node.CreateValueFromData('parent', parent_data, node.GetType())

    def increment_node(self, node):
        max_steps = self.num_children()
        if self.node_ptr_value(self.right(node)) != 0:
            x = self.right(node)
            max_steps -= 1
            while self.node_ptr_value(self.left(x)) != 0:
                x = self.left(x)
                max_steps -= 1
                if max_steps <= 0:
                    self.garbage = True
                    return None
            return x
        else:
            x = node
            y = self.parent(x)
            max_steps -= 1
            while self.node_ptr_value(x) == self.node_ptr_value(self.right(y)):
                x = y
                y = self.parent(y)
                max_steps -= 1
                if max_steps <= 0:
                    self.garbage = True
                    return None
            if self.node_ptr_value(self.right(x)) != self.node_ptr_value(y):
                x = y
            return x

    def update(self):
        try:
            self.garbage = False
            self.root_node = self.valobj.GetChildMemberWithName('d').GetChildMemberWithName(
                'header').GetChildMemberWithName('left')
            self.header = self.valobj.GetChildMemberWithName('d').GetChildMemberWithName('mostLeftNode')
            self.data_type = self.extract_type()
        except:
            pass
        
def qmap_summary(valobj, dict):
    return 'size=' + str(valobj.GetNumChildren())
            
def qstring_summary(v, d):
    try:
        d = v.GetChildMemberWithName('d') 
        offset = d.GetChildMemberWithName('offset').GetValueAsUnsigned()
        size = d.GetChildMemberWithName('size').GetValueAsUnsigned()
        data = '"'
        if size > 0:
            s2 = size * 2
            o2 = offset // 2
            ds = d.type.GetPointeeType().size
            arr = d.GetPointeeData(0, ( offset + s2 + ds ) // ds ).uint16
            for i in range(o2, o2+size):
                data += chr(arr[i])
        data += '"'
        return data
    except:
        print("exception in qstring_summary")
        return "<invalid>"

def qbytearray_summary(v, d):
    try:
        d = v.GetChildMemberWithName('d') # d has type QByteArray::Data*, *d has size 24
        offset = d.GetChildMemberWithName('offset').GetValueAsUnsigned()
        size = d.GetChildMemberWithName('size').GetValueAsUnsigned()
        data = "$"
        if size > 0:
            ds = d.type.GetPointeeType().size
            arr = d.GetPointeeData(0, ( offset + size + ds ) // ds ).uint8
            for i in range(offset, offset+size):
                data += format(arr[i],"x")
        data += "$"
        return data
    except:
        print("exception in qbytearray_summary")
        return "<invalid>"

def __lldb_init_module(debugger, dict):
    debugger.HandleCommand('type summary add -F lldbformatter.qbytearray_summary QByteArray')
    debugger.HandleCommand('type summary add -F lldbformatter.qstring_summary QString')
    debugger.HandleCommand('type summary add -F lldbformatter.qlist_summary -e -x "^QList<.+>$"')
    debugger.HandleCommand('type synthetic add -l lldbformatter.qlist_provider -x "^QList<.+>$"')
    debugger.HandleCommand('type synthetic add -x "^QVector<.+>$" -l lldbformatter.qvector_provider')
    debugger.HandleCommand('type summary add -F lldbformatter.qvector_summary -e -x "^QVector<.+>$"')
    debugger.HandleCommand('type synthetic add -l lldbformatter.qmap_provider -x "^QMap<.+, .+>$"')
    debugger.HandleCommand('type summary add -F lldbformatter.qmap_summary -e -x "^QMap<.+, .+>$"')

