

#include <LibWeb/Bindings/Intrinsics.h>
#include <LibWeb/Bindings/MediaDevicesPrototype.h>
#include <LibWeb/MediaCapture/MediaDevices.h>
#include <LibWeb/WebIDL/Promise.h>

namespace Web::MediaCapture {

GC_DEFINE_ALLOCATOR(MediaDevices);

GC::Ref<MediaDevices> MediaDevices::create(JS::Realm& realm)
{
    return realm.create<MediaDevices>(realm);
}

GC::Ref<WebIDL::Promise> MediaDevices::enumerate_devices()
{
    return WebIDL::create_resolved_promise(realm(), JS::js_nan());
}

MediaDevices::MediaDevices(JS::Realm& realm)
    : DOM::EventTarget(realm)
{
}

void MediaDevices::initialize(JS::Realm& realm)
{
    WEB_SET_PROTOTYPE_FOR_INTERFACE(MediaDevices);
    Base::initialize(realm);
}

void MediaDevices::visit_edges(Visitor& visitor)
{
    Base::visit_edges(visitor);
}

}
