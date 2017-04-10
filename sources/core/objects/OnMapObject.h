#pragma once

#include <list>

#include "OnMapBase.h"
#include "representation/ViewInfo.h"

class IOnMapObject: public IOnMapBase
{
public:
    IOnMapObject(quint32 id);
    DECLARE_SAVED(IOnMapObject, IOnMapBase);
    DECLARE_GET_TYPE_ITEM(ionmapitem);
    virtual bool IsVisibleByPlayer() const override
    {
        return owner.IsValid() && owner->IsVisibleByPlayer();
    }

    virtual PassableLevel GetPassable(Dir direct) const override
    {
        switch (direct)
        {
        case Dir::UP:
            return passable_up;
        case Dir::DOWN:
            return passable_down;
        case Dir::LEFT:
            return passable_left;
        case Dir::RIGHT:
            return passable_right;
        case Dir::ALL:
            return passable_all;
        }
        return false;
    }

    void SetPassable(Dir direct, PassableLevel p)
    {
        switch (direct)
        {
        case Dir::UP:
            passable_up = p;
            break;
        case Dir::DOWN:
            passable_down = p;
            break;
        case Dir::LEFT:
            passable_left = p;
            break;
        case Dir::RIGHT:
            passable_right = p;
            break;
        case Dir::ALL:
            passable_all = p;
            break;
        }
        if (owner)
        {
            owner->UpdatePassable();
        }
    }

    virtual bool IsTransparent() const override
    {
        return transparent;
    }
    
    virtual void Delete() override;
    virtual void Represent() override;
    void SetSprite(const QString& T_SPR);
    void SetState(const QString& name);
    ViewInfo* GetView() { return &view_; }
public:
    int KV_SAVEBLE(v_level);
    PassableLevel KV_SAVEBLE(passable_down);
    PassableLevel KV_SAVEBLE(passable_up);
    PassableLevel KV_SAVEBLE(passable_left);
    PassableLevel KV_SAVEBLE(passable_right);
    PassableLevel KV_SAVEBLE(passable_all);

    PassableLevel KV_SAVEBLE(passable_level);

    bool KV_SAVEBLE(transparent);
    QString KV_SAVEBLE(name);
protected:
    ViewInfo KV_SAVEBLE(view_);
};

ADD_TO_TYPELIST(IOnMapObject);
