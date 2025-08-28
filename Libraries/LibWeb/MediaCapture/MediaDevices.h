#pragma once

#include <LibWeb/DOM/EventTarget.h>

namespace Web::MediaCapture {

class MediaDevices : public DOM::EventTarget {
    WEB_PLATFORM_OBJECT(MediaDevices, DOM::EventTarget);
    GC_DECLARE_ALLOCATOR(MediaDevices);

public:
    static GC::Ref<MediaDevices> create(JS::Realm&);
    GC::Ref<WebIDL::Promise> enumerate_devices();

private:
    MediaDevices(JS::Realm&);

    virtual void initialize(JS::Realm&) override;
    virtual void visit_edges(Visitor&) override;

    HashMap<String, String> m_devices_live_map;
    HashMap<String, String> m_devices_accessible_map;
    HashMap<String, String> m_kinds_accessible;
    Vector<String> m_stored_device_lists;
    bool m_can_expose_camera_info { false };
    bool m_can_expose_microphone { false };
    Vector<String> m_media_stream_track_sources;
};

}
