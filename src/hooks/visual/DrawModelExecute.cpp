/*
  Created by Jenny White on 29.04.18.
  Copyright (c) 2018 nullworks. All rights reserved.
*/

#include <MiscTemporary.hpp>
#include <settings/Bool.hpp>
#include "HookedMethods.hpp"
#include "Backtrack.hpp"
#include <visual/EffectChams.hpp>
#include <visual/EffectGlow.hpp>

static settings::Bool no_arms{ "remove.arms", "false" };
static settings::Bool no_hats{ "remove.hats", "false" };

namespace effect_glow
{
extern settings::Bool enable;
}
namespace effect_chams
{
extern settings::Bool enable;
}
namespace hacks::shared::backtrack
{
extern settings::Bool backtrack_chams_glow;
}
namespace hooked_methods
{

DEFINE_HOOKED_METHOD(DrawModelExecute, void, IVModelRender *this_, const DrawModelState_t &state, const ModelRenderInfo_t &info, matrix3x4_t *bone)
{
    if (!isHackActive())
        return original::DrawModelExecute(this_, state, info, bone);

    if (!(hacks::shared::backtrack::isBacktrackEnabled || spectator_target || no_arms || no_hats || (*clean_screenshots && g_IEngine->IsTakingScreenshot()) || CE_BAD(LOCAL_E) || !LOCAL_E->m_bAlivePlayer()))
    {
        return original::DrawModelExecute(this_, state, info, bone);
    }

    PROF_SECTION(DrawModelExecute);

    if (no_arms || no_hats)
    {
        if (info.pModel)
        {
            const char *name = g_IModelInfo->GetModelName(info.pModel);
            if (name)
            {
                std::string sname = name;
                if (no_arms && sname.find("arms") != std::string::npos)
                {
                    return;
                }
                else if (no_hats && sname.find("player/items") != std::string::npos)
                {
                    return;
                }
            }
        }
    }

    if (hacks::shared::backtrack::isBacktrackEnabled && hacks::shared::backtrack::backtrack_chams_glow && (effect_glow::enable || effect_chams::enable))
    {
        const char *name = g_IModelInfo->GetModelName(info.pModel);
        if (name)
        {
            std::string sname = name;
            if (sname.find("models/player") || sname.find("models/weapons") || sname.find("models/workshop/player") || sname.find("models/workshop/weapons"))
            {

                if (IDX_GOOD(info.entity_index) && info.entity_index < g_IEngine->GetMaxClients() && info.entity_index != g_IEngine->GetLocalPlayer())
                {
                    CachedEntity *ent = ENTITY(info.entity_index);
                    if (CE_GOOD(ent) && ent->m_bAlivePlayer())
                    {
                        // Backup Blend
                        float orig_blend = g_IVRenderView->GetBlend();
                        // Make Backtrack stuff seethrough
                        g_IVRenderView->SetBlend(0.999f);
                        // Get Backtrack data for target entity
                        auto head_pos = hacks::shared::backtrack::headPositions[info.entity_index];
                        // Usable vector instead of ptr to c style array, also used to filter valid and invalid ticks
                        std::vector<hacks::shared::backtrack::BacktrackData> usable;
                        for (int i = 0; i < 66; i++)
                        {
                            if (hacks::shared::backtrack::ValidTick(head_pos[i], ent))
                                usable.push_back(head_pos[i]);
                        }
                        // Crash much?
                        if (usable.size())
                        {
                            // Sort
                            std::sort(usable.begin(), usable.end(), [](hacks::shared::backtrack::BacktrackData &a, hacks::shared::backtrack::BacktrackData &b) { return a.tickcount < b.tickcount; });
                            original::DrawModelExecute(this_, state, info, usable[0].bones);
                        }
                        g_IVRenderView->SetBlend(orig_blend);
                    }
                }
            }
        }
    }
    IClientUnknown *unk = info.pRenderable->GetIClientUnknown();
    if (unk)
    {
        IClientEntity *ent = unk->GetIClientEntity();
        if (ent)
            if (ent->entindex() == spectator_target)
                return;
        if (ent && !effect_chams::g_EffectChams.drawing && effect_chams::g_EffectChams.ShouldRenderChams(ent))
            return;
    }

    return original::DrawModelExecute(this_, state, info, bone);
} // namespace hooked_methods
} // namespace hooked_methods
