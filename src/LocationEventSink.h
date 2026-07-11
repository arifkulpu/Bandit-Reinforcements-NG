#pragma once

#include <RE/Skyrim.h>

class LocationEventSink : 
    public RE::BSTEventSink<RE::TESCellAttachDetachEvent>,
    public RE::BSTEventSink<RE::TESActorLocationChangeEvent>
{
public:
    static LocationEventSink* GetSingleton();
    
    // For dungeon exit ambush
    RE::BSEventNotifyControl ProcessEvent(const RE::TESCellAttachDetachEvent* a_event, 
                                          RE::BSTEventSource<RE::TESCellAttachDetachEvent>* a_eventSource) override;
                                          
    // For location entry reinforcements
    RE::BSEventNotifyControl ProcessEvent(const RE::TESActorLocationChangeEvent* a_event, 
                                          RE::BSTEventSource<RE::TESActorLocationChangeEvent>* a_eventSource) override;
                                          
    void Register();

private:
    // Track the last interior cell to detect dungeon exits (for ambush)
    RE::FormID m_lastInteriorCellID = 0;
    bool m_wasInInterior = false;

    // Prevent processing same cell multiple times in a row
    RE::FormID m_lastProcessedCellID = 0;
};
