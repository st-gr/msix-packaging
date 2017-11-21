#pragma once

#include <string>
#include <vector>
#include <map>
#include <memory>

#include "AppxPackaging.hpp"
#include "AppxWindows.hpp"
#include "Exceptions.hpp"
#include "ComHelper.hpp"
#include "StreamBase.hpp"
#include "StorageObject.hpp"
#include "ZipObject.hpp"
#include "VerifierObject.hpp"
#include "XmlObject.hpp"
#include "AppxBlockMapObject.hpp"
#include "AppxSignature.hpp"
#include "AppxFactory.hpp"

// internal interface
EXTERN_C const IID IID_IAppxPackage;   
#ifndef WIN32
MIDL_INTERFACE("51b2c456-aaa9-46d6-8ec9-298220559189")
interface IAppxPackage : public IUnknown
#else
#include "Unknwn.h"
#include "Objidl.h"
class IAppxPackage : public IUnknown
#endif
{
public:
    #ifdef WIN32
    virtual ~IAppxPackage() {}
    #endif

    virtual void Pack(APPX_PACKUNPACK_OPTION options, const std::string& certFile, IStorageObject* from) = 0;
    virtual void Unpack(APPX_PACKUNPACK_OPTION options, IStorageObject* to) = 0;
    virtual std::vector<std::string>& GetFootprintFiles() = 0;
};

SpecializeUuidOfImpl(IAppxPackage);

namespace xPlat {
    // The 5-tuple that describes the identity of a package
    struct AppxPackageId
    {
        AppxPackageId(
            const std::string& name,
            const std::string& version,
            const std::string& resourceId,
            const std::string& architecture,
            const std::string& publisher);

        std::string Name;
        std::string Version;
        std::string ResourceId;
        std::string Architecture;
        std::string PublisherHash;

        std::string GetPackageFullName()
        {
            return Name + "_" + Version + "_" + Architecture + "_" + ResourceId + "_" + PublisherHash;
        }

        std::string GetPackageFamilyName()
        {
            return Name + "_" + PublisherHash;
        }
    };

    // Object backed by AppxManifest.xml
    class AppxManifestObject : public ComClass<AppxManifestObject, IVerifierObject>
    {
    public:
        AppxManifestObject(IStream* stream);

        // IVerifierObject
        bool HasStream()     override { return m_stream.Get() != nullptr; }
        IStream* GetStream() override { return m_stream.Get(); }
        IStream* GetValidationStream(const std::string& part, IStream* stream) override
        {
            throw Exception(Error::NotSupported);
        }

        AppxPackageId* GetPackageId()    { return m_packageId.get(); }
        std::string GetPackageFullName() { return m_packageId->GetPackageFullName(); }

    protected:
        ComPtr<IStream> m_stream;
        std::unique_ptr<AppxPackageId> m_packageId;
    };

    // Storage object representing the entire AppxPackage
    class AppxPackageObject : public ComClass<AppxPackageObject, IAppxPackageReader, IAppxPackage, IStorageObject>
    {
    public:
        AppxPackageObject(IxPlatFactory* factory, APPX_VALIDATION_OPTION validation, IStorageObject* container);

        // internal IxPlatAppxPackage methods
        void Pack(APPX_PACKUNPACK_OPTION options, const std::string& certFile, IStorageObject* from) override;
        void Unpack(APPX_PACKUNPACK_OPTION options, IStorageObject* to) override;

        // IAppxPackageReader
        HRESULT STDMETHODCALLTYPE GetBlockMap(IAppxBlockMapReader** blockMapReader) override;
        HRESULT STDMETHODCALLTYPE GetFootprintFile(APPX_FOOTPRINT_FILE_TYPE type, IAppxFile** file) override;
        HRESULT STDMETHODCALLTYPE GetPayloadFile(LPCWSTR fileName, IAppxFile** file) override;
        HRESULT STDMETHODCALLTYPE GetPayloadFiles(IAppxFilesEnumerator**  filesEnumerator) override;
        HRESULT STDMETHODCALLTYPE GetManifest(IAppxManifestReader**  manifestReader) override;

        // returns a list of the footprint files found within this appx package.
        std::vector<std::string>& GetFootprintFiles() override { return m_footprintFiles; }

        // IStorageObject methods
        std::string               GetPathSeparator() override;
        std::vector<std::string>  GetFileNames(FileNameOptions options) override;
        IStream*                  GetFile(const std::string& fileName) override;
        void                      RemoveFile(const std::string& fileName) override;
        IStream*                  OpenFile(const std::string& fileName, xPlat::FileStream::Mode mode) override;
        void                      CommitChanges() override;

    protected:
        std::map<std::string, ComPtr<IStream>>  m_streams;   

        APPX_VALIDATION_OPTION      m_validation = APPX_VALIDATION_OPTION::APPX_VALIDATION_OPTION_FULL;
        ComPtr<IxPlatFactory>       m_factory;
        ComPtr<IVerifierObject>     m_appxSignature;
        ComPtr<IVerifierObject>     m_appxBlockMap;
        ComPtr<IVerifierObject>     m_appxManifest;
        ComPtr<IVerifierObject>     m_contentType;        
        ComPtr<IStorageObject>      m_container;
        
        std::vector<std::string>    m_payloadFiles;
        std::vector<std::string>    m_footprintFiles;
    };

    class AppxFilesEnumerator : public xPlat::ComClass<AppxFilesEnumerator, IAppxFilesEnumerator>
    {
    protected:
        ComPtr<IStorageObject>      m_storage;
        std::size_t                 m_cursor = 0;
        std::vector<std::string>    m_files;

    public:
        AppxFilesEnumerator(IStorageObject* storage) : 
            m_storage(storage)
        {
            m_files = storage->GetFileNames(FileNameOptions::PayloadOnly);            
        }

        // IAppxFilesEnumerator
        HRESULT STDMETHODCALLTYPE GetCurrent(IAppxFile** file) override
        {   return ResultOf([&]{
                ThrowErrorIf(Error::InvalidParameter,(file == nullptr || *file != nullptr), "bad pointer");
                ThrowErrorIf(Error::Unexpected, (m_cursor >= m_files.size()), "index out of range");
                *file = ComPtr<IStream>(m_storage->GetFile(m_files[m_cursor])).As<IAppxFile>().Detach();
            });
        }

        HRESULT STDMETHODCALLTYPE GetHasCurrent(BOOL* hasCurrent) override
        {   return ResultOf([&]{
                ThrowErrorIfNot(Error::InvalidParameter, (hasCurrent), "bad pointer");
                *hasCurrent = (m_cursor != m_files.size()) ? TRUE : FALSE;
            });
        }

        HRESULT STDMETHODCALLTYPE MoveNext(BOOL* hasNext) override      
        {   return ResultOf([&]{
                ThrowErrorIfNot(Error::InvalidParameter, (hasNext), "bad pointer");
                *hasNext = (++m_cursor != m_files.size()) ? TRUE : FALSE;
            });
        }
    };
}