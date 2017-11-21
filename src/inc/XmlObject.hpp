#pragma once
#include <memory>
#include <string>
#include <vector>

#include "Exceptions.hpp"
#include "StreamBase.hpp"
#include "VerifierObject.hpp"

// Mandatory for using any feature of Xerces.
#include "xercesc/util/PlatformUtils.hpp"
#include "xercesc/dom/DOM.hpp"

namespace xPlat {

    // internal interface
    EXTERN_C const IID IID_IXmlObject;   
    #ifndef WIN32
    MIDL_INTERFACE("0e7a446e-baf7-44c1-b38a-216bfa18a1a8")
    interface IXmlObject : public IUnknown
    #else
    #include "Unknwn.h"
    #include "Objidl.h"
    class IXmlObject : public IUnknown
    #endif
    // An internal interface for XML
    {
    public:
        #ifdef WIN32
        virtual ~IXmlObject() {}
        #endif            

        virtual void Write() = 0;
        virtual std::shared_ptr<XERCES_CPP_NAMESPACE::DOMDocument> Document() = 0;
    };
    
    SpecializeUuidOfImpl(IXmlObject);    

    // XML de-serialization happens during construction, of this object.
    // XML serialization happens through the Write method
    class XmlObject : public ComClass<XmlObject, IXmlObject, IVerifierObject>
    {
    public:
        // TODO: Implement actual XML validation....
        XmlObject(IStream* stream) : m_stream(stream) {}

        // IXmlObject
        void Write() override { throw Exception(Error::NotImplemented); }
        std::shared_ptr<XERCES_CPP_NAMESPACE::DOMDocument> Document() override { return m_DOMDocument;}

        // IVerifierObject
        bool HasStream() override { return m_stream.Get() != nullptr; }
        IStream* GetStream() override { return m_stream.Get(); }
        IStream* GetValidationStream(const std::string& part, IStream* stream) override
        {
            throw Exception(Error::NotSupported);
        }

    protected:
        ComPtr<IStream> m_stream;
        std::shared_ptr<XERCES_CPP_NAMESPACE::DOMImplementation> m_DOMImplementation;
        std::shared_ptr<XERCES_CPP_NAMESPACE::DOMDocument>       m_DOMDocument;
    };

} // namespace xPlat

