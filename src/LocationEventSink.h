#pragma once

#include <RE/Skyrim.h>

class LocationEventSink : public RE::BSTEventSink<RE::TESCellAttachDetachEvent> {
public:
    static LocationEventSink* GetSingleton();
    RE::BSEventNotifyControl ProcessEvent(const RE::TESCellAttachDetachEvent* a_event, 
                                          RE::BSTEventSource<RE::TESCellAttachDetachEvent>* a_eventSource) override;
    void Register();

private:
    // Track the last interior cell to detect dungeon exits (for ambush)
    RE::FormID m_lastInteriorCellID = 0;
    bool m_wasInInterior = false;

    // Prevent processing same cell multiple times in a row
    RE::FormID m_lastProcessedCellID = 0;
};
