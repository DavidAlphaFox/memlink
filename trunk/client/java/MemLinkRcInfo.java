/* ----------------------------------------------------------------------------
 * This file was automatically generated by SWIG (http://www.swig.org).
 * Version 1.3.40
 *
 * Do not make changes to this file unless you know what you are doing--modify
 * the SWIG interface file instead.
 * ----------------------------------------------------------------------------- */

package com.googlecode.memlink;

public class MemLinkRcInfo {
  private long swigCPtr;
  protected boolean swigCMemOwn;

  protected MemLinkRcInfo(long cPtr, boolean cMemoryOwn) {
    swigCMemOwn = cMemoryOwn;
    swigCPtr = cPtr;
  }

  protected static long getCPtr(MemLinkRcInfo obj) {
    return (obj == null) ? 0 : obj.swigCPtr;
  }

  protected void finalize() {
    delete();
  }

  public synchronized void delete() {
    if (swigCPtr != 0) {
      cmemlink.memlink_rcinfo_free(this);
      if (swigCMemOwn) {
        swigCMemOwn = false;
        cmemlinkJNI.delete_MemLinkRcInfo(swigCPtr);
      }
      swigCPtr = 0;
    }
  }

  public void setConncount(int value) {
    cmemlinkJNI.MemLinkRcInfo_conncount_set(swigCPtr, this, value);
  }

  public int getConncount() {
    return cmemlinkJNI.MemLinkRcInfo_conncount_get(swigCPtr, this);
  }

  public void setRoot(MemLinkRcItem value) {
    cmemlinkJNI.MemLinkRcInfo_root_set(swigCPtr, this, MemLinkRcItem.getCPtr(value), value);
  }

  public MemLinkRcItem getRoot() {
    long cPtr = cmemlinkJNI.MemLinkRcInfo_root_get(swigCPtr, this);
    return (cPtr == 0) ? null : new MemLinkRcItem(cPtr, false);
  }

  public MemLinkRcInfo() {
    this(cmemlinkJNI.new_MemLinkRcInfo(), true);
  }

}