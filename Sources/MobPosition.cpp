#include "MobPosition.h"

#include "Manager.h"
#include "constheader.h"
#include "Tile.h"
#include "Creator.h"

namespace mob_position 
{
    int get_mob_z()
    {
        if (GetMob().valid())
            return GetMob()->GetZ();
        return 0;
    }

}
