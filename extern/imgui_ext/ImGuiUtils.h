#pragma once

#include "IconsFontAwesome.h" // from https://github.com/juliettef/IconFontCppHeaders

namespace ImGui
{

    //for usage with std::string and not char*. Taken from https://github.com/emilk/emilib/blob/master/emilib/imgui_helpers.cpp
    bool InputText(const std::string& label, std::string& text,  ImGuiInputTextFlags flags = 0, ImGuiInputTextCallback callback = NULL, void* user_data = NULL){
        char buff[1024];
        strncpy(buff, text.c_str(), sizeof(buff));
        if (ImGui::InputText(label.c_str(), buff, sizeof(buff), flags, callback, user_data)) {
            text = buff;
            return true;
        } else {
            return false;
        }
    }

    
   
    inline bool CheckBoxFont( const char* name_, bool* pB_, const char* pOn_ = "[X]", const char* pOff_="[  ]" )
    {
        if( *pB_ )
        {
            ImGui::Text(pOn_);
        }
        else
        {
            ImGui::Text(pOff_);
        }
        bool bHover = false;
        bHover = bHover || ImGui::IsItemHovered();
        ImGui::SameLine();
        ImGui::Text( name_ );
        bHover = bHover || ImGui::IsItemHovered();
        if( bHover && ImGui::IsMouseClicked(0) )
        {
            *pB_ = ! *pB_;
            return true;
        }
        return false;
    }

    inline bool CheckBoxTick( const char* name_, bool* pB_ )
    {
        return CheckBoxFont( name_, pB_, ICON_FA_CHECK_SQUARE_O, ICON_FA_SQUARE_O );
    }

    inline bool MenuItemCheckbox( const char* name_, bool* pB_ )
    {
        bool retval = ImGui::MenuItem( name_ );
        ImGui::SameLine();
        if( *pB_ )
        {
            ImGui::Text(ICON_FA_CHECK_SQUARE_O);
        }
        else
        {
            ImGui::Text(ICON_FA_SQUARE_O);
        }
        if( retval )
        {
            *pB_ = ! *pB_;
        }
        return retval;
    }

    struct FrameTimeHistogram
    {
        // configuration params - modify these at will
        static const int NUM = 101; //last value is from T-1 to inf.

        float  dT      = 0.001f;    // in seconds, default 1ms
        float  refresh = 1.0f/60.0f;// set this to your target refresh rate

        static const int NUM_MARKERS = 2;
        float  markers[NUM_MARKERS] = { 0.99f, 0.999f };

        // data
        ImVec2 size    = ImVec2( 3.0f * NUM, 40.0f );
        float  lastdT  = 0.0f;
        float  timesTotal;
        float  countsTotal;
        float  times[ NUM];
        float  counts[NUM];
        float  hitchTimes[ NUM];
        float  hitchCounts[NUM];

        FrameTimeHistogram()
        {
            Clear();
        }

        void Clear()
        {
            timesTotal  = 0.0f;
            countsTotal = 0.0f;
            memset(times,       0, sizeof(times) );
            memset(counts,      0, sizeof(counts) );
            memset(hitchTimes,  0, sizeof(hitchTimes) );
            memset(hitchCounts, 0, sizeof(hitchCounts) );
        }

        int GetBin( float time_ )
        {
            int bin = (int)floor( time_ / dT  );
            if( bin >= NUM )
            {
                bin = NUM - 1;
            }
            return bin;
        }

        void Update( float deltaT_ )
        {
            if( deltaT_ < 0.0f )
            {
                assert(false);
                return;
            }
            int bin = GetBin( deltaT_ );
            times[ bin] += deltaT_;
            timesTotal  += deltaT_;
            counts[bin] += 1.0f;
            countsTotal += 1.0f;

            float hitch = abs( lastdT - deltaT_ );
            int deltaBin = GetBin( hitch );
            hitchTimes[ deltaBin] += hitch;
            hitchCounts[deltaBin] += 1.0f;
            lastdT = deltaT_;
        }
        
        void PlotRefreshLines( float total_ = 0.0f, float* pValues_ = NULL)
        {
            ImDrawList* draw = ImGui::GetWindowDrawList();
            const ImGuiStyle& style = ImGui::GetStyle();
            ImVec2 pad              = style.FramePadding;
            ImVec2 min              = ImGui::GetItemRectMin();
            min.x += pad.x;
            ImVec2 max              = ImGui::GetItemRectMax();
            max.x -= pad.x;

            float xRefresh          = (max.x - min.x) * refresh / ( dT * NUM );

            float xCurr             = xRefresh + min.x;
            while( xCurr < max.x )
            {
                float xP = ceil( xCurr ); // use ceil to get integer coords or else lines look odd
                draw->AddLine( ImVec2( xP, min.y ), ImVec2( xP, max.y ), 0x50FFFFFF );
                xCurr += xRefresh;
            }

            if( pValues_ )
            {
                // calc markers
                float currTotal = 0.0f;
                int   mark      = 0;
                for( int i = 0; i < NUM && mark < NUM_MARKERS; ++i )
                {
                    currTotal += pValues_[i];
                    if( total_ * markers[mark] < currTotal )
                    {
                        float xP = ceil( (float)(i+1)/(float)NUM * ( max.x - min.x ) + min.x ); // use ceil to get integer coords or else lines look odd
                        draw->AddLine( ImVec2( xP, min.y ), ImVec2( xP, max.y ), 0xFFFF0000 );
                        ++mark;
                    }
                }
            }
        }

        void CalcHistogramSize( int numShown_ )
        {
            ImVec2 wRegion = ImGui::GetContentRegionMax();
            float heightGone = 7.0f * ImGui::GetItemsLineHeightWithSpacing();
            wRegion.y -= heightGone;
            wRegion.y /= (float) numShown_;
            const ImGuiStyle& style = ImGui::GetStyle();
            ImVec2 pad              = style.FramePadding;
            wRegion.x -= 2.0f * pad.x;
            size = wRegion;
        }
        

        void Draw(const char* name_, bool* pOpen_ = NULL )
        {
            if (ImGui::Begin( name_, pOpen_ ))
            {
                int numShown = 0;
                if(ImGui::CollapsingHeader("Time Histogram"))
                {
                    ++numShown;
                    ImGui::PlotHistogram("", times,   NUM, 0, NULL, FLT_MAX, FLT_MAX, size );
                    PlotRefreshLines( timesTotal, times );
                }
                if(ImGui::CollapsingHeader("Count Histogram"))
                {
                    ++numShown;
                    ImGui::PlotHistogram("", counts, NUM, 0, NULL, FLT_MAX, FLT_MAX, size );
                    PlotRefreshLines( countsTotal, counts );
                }
                if(ImGui::CollapsingHeader("Hitch Time Histogram"))
                {
                    ++numShown;
                    ImGui::PlotHistogram("", hitchTimes,   NUM, 0, NULL, FLT_MAX, FLT_MAX, size );
                    PlotRefreshLines();
                }
                if(ImGui::CollapsingHeader("Hitch Count Histogram"))
                {
                    ++numShown;
                    ImGui::PlotHistogram("", hitchCounts, NUM, 0, NULL, FLT_MAX, FLT_MAX, size );
                    PlotRefreshLines();
                }
                if( ImGui::Button("Clear") )
                {
                    Clear();
                }
                CalcHistogramSize( numShown ); 
            }
            ImGui::End();
        }
    };

};
