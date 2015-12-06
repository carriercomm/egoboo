#include "object_physics.h"
#include "game/game.h"
#include "game/player.h"
#include "game/renderer_2d.h"
#include "egolib/Graphics/ModelDescriptor.hpp"
#include "game/Physics/PhysicalConstants.hpp"
#include "game/Core/GameEngine.hpp"

#include "game/Entities/_Include.hpp"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
void move_one_character_get_environment( Object * pchr )
{
    if (!pchr || pchr->isTerminated()) return;
    chr_environment_t& enviro = pchr->enviro;

    // determine if the character is standing on a platform
    Object *pplatform = nullptr;
    if ( _currentModule->getObjectHandler().exists( pchr->onwhichplatform_ref ) )
    {
        pplatform = _currentModule->getObjectHandler().get( pchr->onwhichplatform_ref );
    }

    ego_mesh_t *mesh = _currentModule->getMeshPointer().get();

    //---- character "floor" level
    float grid_level = mesh->getElevation(Vector2f(pchr->getPosX(), pchr->getPosY()), false);

    // chr_set_enviro_grid_level() sets up the reflection level and reflection matrix
    if (grid_level != pchr->enviro.grid_level) {
        pchr->enviro.grid_level = grid_level;

        chr_instance_t::apply_reflection_matrix(pchr->inst, grid_level);
    }

    // The actual level of the floor underneath the character.
    if (pplatform)
    {
        enviro.floor_level = pplatform->getPosZ() + pplatform->chr_min_cv._maxs[OCT_Z];
    }
    else
    {
        enviro.floor_level = pchr->getAttribute(Ego::Attribute::WALK_ON_WATER) > 0 ? mesh->getElevation(Vector2f(pchr->getPosX(), pchr->getPosY()), true) : grid_level;
    }

    //---- The actual level of the characer.
    //     Estimate platform attachment from whatever is in the onwhichplatform_ref variable from the
    //     last loop
    if (pplatform)
    {
        enviro.level = pplatform->getPosZ() + pplatform->chr_min_cv._maxs[OCT_Z];
    }
    else {
        enviro.level = enviro.floor_level;
    }

    //---- The flying height of the character, the maximum of tile level, platform level and water level
    if (pchr->isOnWaterTile())
    {
        enviro.fly_level = std::max(enviro.level, water._surface_level);
    }

    // fly above pits...
    if (enviro.fly_level < 0)
    {
        enviro.fly_level = 0;
    }

    // set the zlerp
    enviro.zlerp = (pchr->getPosZ() - enviro.level) / PLATTOLERANCE;
    enviro.zlerp = Ego::Math::constrain(enviro.zlerp, 0.0f, 1.0f);

    enviro.grounded = !pchr->isFlying() && enviro.zlerp <= 0.25f;

    // the "watery-ness" of whatever water might be here
    enviro.is_slippy = !(water._is_water && enviro.inwater) && (0 != mesh->test_fx(pchr->getTile(), MAPFX_SLIPPY));

    //---- jump stuff
    if ( pchr->isFlying() )
    {
        // Flying
        pchr->jumpready = false;
    }
    else
    {
        // Character is in the air
        pchr->jumpready = enviro.grounded;

        // Down jump timer
        if ((pchr->isBeingHeld() || pchr->jumpready || pchr->jumpnumber > 0) && pchr->jump_timer > 0) { 
            pchr->jump_timer--;
        }

        // Do ground hits
        if ( enviro.grounded && pchr->vel[kZ] < -Ego::Physics::STOP_BOUNCING && pchr->hitready )
        {
            SET_BIT( pchr->ai.alert, ALERTIF_HITGROUND );
            pchr->hitready = false;
        }

        if ( enviro.grounded && 0 == pchr->jump_timer )
        {
            // Reset jumping on flat areas of slippiness
            if(!enviro.is_slippy || g_meshLookupTables.twist_flat[mesh->get_twist(pchr->getTile())]) {
                pchr->jumpnumber = pchr->getAttribute(Ego::Attribute::NUMBER_OF_JUMPS);                
            }
        }
    }
}

//--------------------------------------------------------------------------------------------
egolib_rv chr_update_collision_size( Object * pchr, bool update_matrix )
{
    /// @author BB
    ///
    /// @details use this function to update the pchr->chr_max_cv and  pchr->chr_min_cv with
    ///       values that reflect the best possible collision volume
    ///
    /// @note This function takes quite a bit of time, so it must only be called when the
    /// vertices change because of an animation or because the matrix changes.
    ///
    /// @todo it might be possible to cache the src[] array used in this function.
    /// if the matrix changes, then you would not need to recalculate this data...

    Vector4f  src[16];  // for the upper and lower octagon points
    Vector4f  dst[16];  // for the upper and lower octagon points

    if ( nullptr == pchr ) return rv_error;

    // re-initialize the collision volumes
    pchr->chr_min_cv = oct_bb_t();
    pchr->chr_max_cv = oct_bb_t();
    for ( size_t cnt = 0; cnt < SLOT_COUNT; cnt++ )
    {
        pchr->slot_cv[cnt] = oct_bb_t();
    }

    // make sure the matrix is updated properly
    if ( update_matrix )
    {
        // call chr_update_matrix() but pass in a false value to prevent a recursive call
        if ( rv_error == chr_update_matrix( pchr, false ) )
        {
            return rv_error;
        }
    }

    // make sure the bounding box is calculated properly
    if (gfx_error == chr_instance_t::update_bbox(pchr->inst)) {
        return rv_error;
    }

    // convert the point cloud in the GLvertex array (pchr->inst.vrt_lst) to
    // a level 1 bounding box. Subtract off the position of the character
    oct_bb_t bsrc = pchr->inst.bbox;

    // convert the corners of the level 1 bounding box to a point cloud
    // keep track of the actual number of vertices, in case the object is square
    int vcount = oct_bb_t::to_points(bsrc, src, 16);

    // transform the new point cloud
    Utilities::transform(pchr->inst.matrix, src, dst, vcount);

    // convert the new point cloud into a level 1 bounding box
    oct_bb_t bdst;
    oct_bb_t::points_to_oct_bb(bdst, dst, vcount);

    //---- set the bounding boxes
    pchr->chr_min_cv = bdst;
    pchr->chr_max_cv = bdst;

    oct_bb_t bmin;
    bmin.assign(pchr->bump);

    // only use pchr->bump.size if it was overridden in data.txt through the [MODL] expansion
    if ( pchr->getProfile()->getBumpOverrideSize() )
    {
        pchr->chr_min_cv.cut(bmin, OCT_X);
        pchr->chr_min_cv.cut(bmin, OCT_Y);

        pchr->chr_max_cv.join(bmin, OCT_X);
        pchr->chr_max_cv.join(bmin, OCT_Y);
    }

    // only use pchr->bump.size_big if it was overridden in data.txt through the [MODL] expansion
    if ( pchr->getProfile()->getBumpOverrideSizeBig() )
    {
        pchr->chr_min_cv.cut(bmin, OCT_XY);
        pchr->chr_min_cv.cut(bmin, OCT_YX);

        pchr->chr_max_cv.join(bmin, OCT_XY);
        pchr->chr_max_cv.join(bmin, OCT_YX);
    }

    // only use pchr->bump.height if it was overridden in data.txt through the [MODL] expansion
    if ( pchr->getProfile()->getBumpOverrideHeight() )
    {
        pchr->chr_min_cv.cut(bmin, OCT_Z);
        pchr->chr_max_cv.join(bmin, OCT_Z);
    }

    //// raise the upper bound for platforms
    //if ( pchr->platform )
    //{
    //    pchr->chr_max_cv.maxs[OCT_Z] += PLATTOLERANCE;
    //}

    //This makes it easier to jump on top of mounts
    if(pchr->isMount()) {
       pchr->chr_max_cv._maxs[OCT_Z] = std::min<float>(MOUNTTOLERANCE, pchr->chr_max_cv._maxs[OCT_Z]);
       pchr->chr_min_cv._maxs[OCT_Z] = std::min<float>(MOUNTTOLERANCE, pchr->chr_min_cv._maxs[OCT_Z]);
    }

    // calculate collision volumes for various slots
    for ( size_t cnt = 0; cnt < SLOT_COUNT; cnt++ )
    {
        if ( !pchr->getProfile()->isSlotValid( static_cast<slot_t>(cnt) ) ) continue;

        chr_calc_grip_cv( pchr, GRIP_LEFT, &pchr->slot_cv[cnt], false );

        pchr->chr_max_cv.join(pchr->slot_cv[cnt]);
    }

    // convert the level 1 bounding box to a level 0 bounding box
    oct_bb_t::downgrade(bdst, pchr->bump_stt, pchr->bump, pchr->bump_1);

    return rv_success;
}
