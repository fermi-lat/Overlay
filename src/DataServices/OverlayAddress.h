#ifndef OverlayAddress_H
#define OverlayAddress_H 1

#include <string>
#include <vector>
#include "GaudiKernel/StatusCode.h"
#include "GaudiKernel/IOpaqueAddress.h"

class IRegistry;
class IOverlayDataSvc;

extern const long int EXCEL_StorageType;

/** @class OverlayAddress
 * @brief Implementation of the IOpaqueAddress class for the Overlays
 *
 * Based on SICb's OverlayAddress class.
 *
 * $Header: /nfs/slac/g/glast/ground/cvs/GlastSvc/src/EventSelector/OverlayAddress.h,v 1.6 2006/03/21 01:26:08 usher Exp $
 */
class OverlayAddress : public IOpaqueAddress   
{
public:
    /// Dummy constructor
    OverlayAddress() 
      : m_refCount(0), 
        m_svcType(0),
        m_clID(0),
        m_pRegistry(0),
        m_dataSvc(0)
    {
      m_ipar[0]=m_ipar[1]=0xFFFFFFFF;
    }

    /// Copy constructor
    OverlayAddress(const OverlayAddress& copy)  
      : IOpaqueAddress(copy),
        m_refCount(0),
        m_svcType(copy.m_svcType),
        m_clID(copy.m_clID),
        m_pRegistry(copy.m_pRegistry),
        m_dataSvc(copy.m_dataSvc)
    {
      m_par[0]  = copy.m_par[0];
      m_par[1]  = copy.m_par[1];
      m_ipar[0] = copy.m_ipar[0];
      m_ipar[1] = copy.m_ipar[1];
    }

    /// Standard constructor
    OverlayAddress(unsigned char      svc,
                   const CLID&        clid, 
                   IOverlayDataSvc*   dataSvc,
                   const std::string& path1 = "",
                   const std::string& path2 = "",
                   unsigned long      ip1   = 0,
                   unsigned long      ip2   = 0)
        : m_refCount(0),
          m_svcType(svc),
          m_clID(clid),
          m_pRegistry(0),
          m_dataSvc(dataSvc)
    {
        m_par[0]  = path1;
        m_par[1]  = path2;
        m_ipar[0] = ip1;
        m_ipar[1] = ip2;
    }

    virtual ~OverlayAddress()    {}

    /// Add reference to object
    virtual unsigned long addRef   ()              {return ++m_refCount;}
    
    /// release reference to object
    virtual unsigned long release  ()   
    {
      int cnt = --m_refCount;
      if ( 0 == cnt ) {delete this;}
      return cnt;
    }
    
    /// Pointer to directory
    virtual IRegistry* registry()            const {return m_pRegistry;}
    
    /// Set pointer to directory
    virtual void setRegistry(IRegistry* pRegistry) {m_pRegistry = pRegistry;}
    
    /// Access : Retrieve class ID of the link
    const CLID& clID()                       const {return m_clID;}
    
    /// Access : Set class ID of the link
    void setClID(const CLID& clid)                 {m_clID = clid;}
    
    /// Access : retrieve the storage type of the class id
    long svcType()                           const {return m_svcType;}
    
    /// Access : set the storage type of the class id
    void setSvcType(long typ)                      {m_svcType = typ;}
    
    /// Retrieve string parameters
    virtual const std::string* par()         const {return m_par;}
    
    /// Retrieve integer parameters
    virtual const unsigned long* ipar()      const {return m_ipar;}

    IOverlayDataSvc* getInputDataSvc()             {return m_dataSvc;}

private:
    /// Reference count
    unsigned long    m_refCount;
  
    /// Storage type
    long             m_svcType;
  
    /// Class id
    CLID             m_clID;
  
    /// String parameters to be accessed
    std::string      m_par[2];
  
    /// Integer parameters to be accessed
    unsigned long    m_ipar[2];
  
    /// Pointer to corresponding directory
    IRegistry*       m_pRegistry;

    /// Pointer to the Overlay Data service
    IOverlayDataSvc* m_dataSvc;
};


#endif  // OverlayAddress_H
