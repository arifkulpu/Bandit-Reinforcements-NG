#pragma once

#include <RE/Skyrim.h>

class LocationEventSink : public RE::BSTEventSink<RE::TESCellAttachDetachEvent> {
public:
    static LocationEventSink* GetSingleton();
    RE::BSEventNotifyControl ProcessEvent(const RE::TESCellAttachDetachEvent* a_event, RE::BSTEventSource<RE::TESCellAttachDetachEvent>* a_eventSource) override;
    
    void Register();
};
