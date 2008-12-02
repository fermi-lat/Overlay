#ifndef _IEventCnvSvc_H
#define _IEventCnvSvc_H 1

#include <string>
#include "GaudiKernel/IInterface.h"

/** @class IEventCnvSvc
* @brief The event conversion service needs an additional interface used 
* by the converters to declare their object path.
*
* Based on ISicbEventCnvSvc
*
* $Header: /nfs/slac/g/glast/ground/cvs/GlastSvc/src/EventSelector/IEventCnvSvc.h,v 1.3 2002/09/06 14:40:00 heather Exp $
*/
class IEventCnvSvc : virtual public IInterface      {
public:
  class Leaf : public std::vector<Leaf*>    {
  public:
    long userParameter;
    std::string path;
    std::string bank;
    CLID        clid;
    Leaf(const std::string& p, const CLID& c, const std::string& b, long u)
      : path(p), bank(b), clid(c), userParameter(u) {}
    Leaf(const Leaf& copy) 
      : path(copy.path), bank(copy.bank), clid(copy.clid), userParameter(copy.userParameter) {}
    Leaf& operator=(const Leaf& copy)   {
      path = copy.path;
      bank = copy.bank;
      clid = copy.clid;
      userParameter = copy.userParameter;
      return *this;
    }
    bool operator==(const Leaf& copy) const  {
      return  path          == copy.path && 
              bank          == copy.bank && 
              clid          == copy.clid && 
              userParameter == copy.userParameter;
    }
  };
  /// Declare target leaf
  virtual StatusCode declareObject(const Leaf& leaf) = 0;
};


#endif // _IEventCnvSvc_H
