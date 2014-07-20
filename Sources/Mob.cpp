#include "Mob.h"

#include <memory>

#include "MapClass.h"
#include "LiquidHolder.h"
#include "Turf.h"
#include "Text.h"

#include "mob_position.h"

#include "NetClientImpl.h"
#include "EffectSystem.h"
#include "MoveEffect.h"
#include "sync_random.h"
#include "ItemFabric.h"
#include "MagicStrings.h"
#include "TileInt.h"
#include "Debug.h"
#include "Params.h"

#include "sound.h"
#include "Creator.h"
#include "SdlInit.h"
#include "MobInt.h"
#include "utils.h"
#include "ImageLoader.h"
#include "SoundLoader.h"
#include "Chat.h"

int ping_send;

void Manager::checkMove(Dir direct)
{
    moveEach(direct);
};


void Manager::touchEach(Dir direct)
{
    GetMob()->dMove = direct;
}

void Manager::moveEach(Dir direct)
{
    undoCenterMove(direct);
};

void Manager::undoCenterMove(Dir direct)
{
    //TODO
    for (int z = 0; z < GetMapMaster()->GetMapD(); ++z)
        for(int x = std::max(0, castTo<CubeTile>(GetMob()->GetOwner().ret_item())->posx() - sizeHsq); 
            x <= std::min(castTo<CubeTile>(GetMob()->GetOwner().ret_item())->posx() + sizeHsq, GetMapMaster()->GetMapH() - 1); x++)
        {
            for(int y = std::max(0, castTo<CubeTile>(GetMob()->GetOwner().ret_item())->posy() - sizeWsq); 
                y <= std::min(castTo<CubeTile>(GetMob()->GetOwner().ret_item())->posy() + sizeWsq, GetMapMaster()->GetMapW() - 1); y++)
            {
                GetMapMaster()->squares[x][y][z]->ForEach([&](id_ptr_on<IOnMapBase> item)
                {
                    Move* eff = EffectFabricOf<Move>::getEffectOf();
                    eff->Init(TITLE_SIZE, direct, GetMob()->pixSpeed, item);
                    eff->Start();
                });
                auto trf = GetMapMaster()->squares[x][y][z]->GetTurf();
                if (trf.valid())
                {
                    Move* eff = EffectFabricOf<Move>::getEffectOf();
                    eff->Init(TITLE_SIZE, direct, GetMob()->pixSpeed, trf);
                    eff->Start();
                }
            }
        }
};

Manager::Manager(std::string adrs)
{
    adrs_ = adrs;
    auto_player_ = false;
    visiblePoint = new std::list<point>;
    isMove = false;
    done = 0;
    pause = false;
    last_fps = FPS_MAX;
};

void Manager::UpdateVisible() 
{
    visiblePoint->clear();
    visiblePoint = 
        GetMapMaster()->losf.calculateVisisble(visiblePoint, 
            castTo<CubeTile>(GetMob()->GetOwner().ret_item())->posx(), 
            castTo<CubeTile>(GetMob()->GetOwner().ret_item())->posy(),
            castTo<CubeTile>(GetMob()->GetOwner().ret_item())->posz());
}

void Manager::process()
{
    GetMapMaster()->numOfPathfind = 0;
    SDL_Color color = {255, 255, 255, 0};

    int begin_of_process;

    int delay = 0;
    int lastTimeFps = SDL_GetTicks();
    int lastTimeC   = SDL_GetTicks();
    fps = 0;
    bool process_in = false;
    while(done == 0)
    { 

        processInput();
        if(NetClient::GetNetClient()->Ready() && !pause)
        {
            process_in = true;
            process_in_msg();
            MAIN_TICK++;
        }

        if(process_in && !pause)
        {
            numOfDeer = 0;
            begin_of_process = SDL_GetTicks();
            GetItemFabric()->foreachProcess();
            GetItemFabric()->Sync();
            //SYSTEM_STREAM << "Processing take: " << (SDL_GetTicks() - begin_of_process) / 1000.0 << "s" << std::endl;
        }
         
        if (!NODRAW)
        {         
            GetMapMaster()->Draw();
            
            FabricProcesser::Get()->process();
            
            ClearGUIZone(); 

            Chat::GetChat()->Process();

            GetMob()->processGUI();
            
            GetTexts().Process();
                       
            GetScreen()->Swap();
        }

        if((SDL_GetTicks() - lastTimeFps) >= 1000 && !pause)
        {
            UpdateVisible();

            if(!(fps > FPS_MAX - 10 && fps < FPS_MAX - 10))
            delay = (int)(1000.0 / FPS_MAX + delay - 1000.0 / fps);
            lastTimeFps = SDL_GetTicks();
            last_fps = fps;
            fps = 0;
          

            GetMapMaster()->numOfPathfind = 0;
        }
        ++fps;
        process_in = false;
        if (NetClient::GetNetClient()->Process() == false)
        {
            SYSTEM_STREAM << "Fail receive messages" << std::endl;
            SDL_Delay(10000);
            break;
        }
    }
};

void Manager::ClearGUIZone()
{
    glColor3f(0.8f, 0.8f, 0.8f);
    glDisable(GL_TEXTURE_2D);
    glBegin(GL_QUADS);
        glVertex2i(sizeW,                0);
        glVertex2i(sizeW,            sizeH);
        glVertex2i(sizeW + guiShift, sizeH);
        glVertex2i(sizeW + guiShift,     0);
    glEnd();
    glEnable(GL_TEXTURE_2D);
}

void Manager::checkMoveMob()
{
};

#define SEND_KEY_MACRO(key) \
      if((auto_player_ && (rand() % 100 == 1)) || (!NODRAW && keys[key])) \
      { \
          if(SDL_GetTicks() - lastShoot >= 80) \
          { \
              Message msg; \
              msg.text = #key; \
              NetClient::GetNetClient()->Send(msg); \
              lastShoot = SDL_GetTicks(); \
          } \
      }

void Manager::processInput()
{
    static Uint8* keys;
    int lastShoot = 0;
    int click_timer = 0;
    if (!NODRAW)
    {
        SDL_Event event;    
        while (SDL_PollEvent(&event))
        { 
            if(event.type == SDL_QUIT) 
                done = 1; 
            if(event.type == SDL_KEYUP)
            {
                if (event.key.keysym.sym == SDLK_o) 
                    pause = !pause;
                if (event.key.keysym.sym == SDLK_F12)
                    ToogleAutoplay();
                if(event.key.keysym.sym == SDLK_F2)
                {
                    ping_send = SDL_GetTicks();
                    Message msg;
                    msg.text = "SDLK_F2";
                    NetClient::GetNetClient()->Send(msg);
                }
            }
            if(event.type == SDL_MOUSEBUTTONDOWN)
            {
                if ((SDL_GetTicks() - click_timer) > 333)
                {
                    click_timer = SDL_GetTicks();
                    auto item = GetMapMaster()->click(event.button.x, event.button.y);
                    if (item.valid())
                    {
                        Message msg;
                        msg.from = item.ret_id();
                        msg.text = "SDL_MOUSEBUTTONDOWN";
                        NetClient::GetNetClient()->Send(msg);
                        last_touch = item->name;
                    }
                }
  //              PlaySound("click.ogx");
            }
            if (event.type == SDL_VIDEORESIZE)
            {
                int max_scale = std::max((event.resize.w / 3), (event.resize.h / 2));

                int new_w = max_scale * 3;
                int new_h = max_scale * 2;
                GetScreen()->ResetScreen(new_w, new_h, 32, SDL_OPENGL | SDL_RESIZABLE);
            }
        }

        SDL_PumpEvents();
        keys = SDL_GetKeyState(NULL);
        /*if(keys[SDLK_h])
        {
            int locatime = SDL_GetTicks();
            auto itr = map->squares[thisMob->posx][thisMob->posy].begin();
            int i = 0;
            while(itr != map->squares[thisMob->posx][thisMob->posy].end())
            {
                SYSTEM_STREAM << i <<": Level " << (*itr)->level;
                itr++;
                i++;
            };
            SYSTEM_STREAM << "Num item: " << i << " in " << (SDL_GetTicks() - locatime) * 1.0 / 1000 << " sec" << std::endl;
        }*/
        if(keys[SDLK_F5])
        {
            int locatime = SDL_GetTicks();
            GetItemFabric()->saveMap("clientmap.map");
            SYSTEM_STREAM << "Map saved in "<< (SDL_GetTicks() - locatime) * 1.0 / 1000 << " second" << std::endl;
        }
        if(keys[SDLK_F6])
        {
            int locatime = SDL_GetTicks();
            GetItemFabric()->clearMap();
            GetItemFabric()->loadMap("clientmap.map");
            SYSTEM_STREAM << "Map load in " << (SDL_GetTicks() - locatime) * 1.0 / 1000 << " second" << std::endl;
        }
        if(keys[SDLK_h])
        {
            SYSTEM_STREAM << "World's hash: " << GetItemFabric()->hash_all() << std::endl; 
        }
    }


    SEND_KEY_MACRO(SDLK_SPACE);
    SEND_KEY_MACRO(SDLK_UP);
    SEND_KEY_MACRO(SDLK_DOWN);
    SEND_KEY_MACRO(SDLK_LEFT);
    SEND_KEY_MACRO(SDLK_RIGHT);
    SEND_KEY_MACRO(SDLK_j);
    SEND_KEY_MACRO(SDLK_p);
    SEND_KEY_MACRO(SDLK_q);
    SEND_KEY_MACRO(SDLK_f);
    SEND_KEY_MACRO(SDLK_a);
    SEND_KEY_MACRO(SDLK_s);
    SEND_KEY_MACRO(SDLK_d);
    SEND_KEY_MACRO(SDLK_w);
    SEND_KEY_MACRO(SDLK_1);
    SEND_KEY_MACRO(SDLK_2);
    SEND_KEY_MACRO(SDLK_3);
    SEND_KEY_MACRO(SDLK_4);
    SEND_KEY_MACRO(SDLK_d);
    SEND_KEY_MACRO(SDLK_e);
    SEND_KEY_MACRO(SDLK_c);
};

void Manager::initWorld()
{
    tick_recv = 0;
    isMove = 0;
    
    if (!InitSDL())
    {
        SYSTEM_STREAM << "Fail SDL load" << std::endl;
    }
    SDL_WM_SetCaption(Debug::GetUniqueName().c_str(), Debug::GetUniqueName().c_str());

    SetManager(this);

    SetItemFabric(new ItemFabric);
    SetMapMaster(new MapMaster);
    if (!NODRAW)
        SetScreen(new Screen(sizeW + guiShift, sizeH));
    SetTexts(new TextPainter);
    SetSpriter(new ASprClass);

    LoadImages();
    LoadSounds();

    NetClient::Init();

    int x = GetParamsHolder().GetParamBool("map_x") ? GetParamsHolder().GetParam<int>("map_x") : 40;
    int y = GetParamsHolder().GetParamBool("map_y") ? GetParamsHolder().GetParam<int>("map_y") : 40;
    int z = GetParamsHolder().GetParamBool("map_z") ? GetParamsHolder().GetParam<int>("map_z") : 2;
    GetMapMaster()->makeTiles(x, y, z);

    if (   !GetParamsHolder().GetParamBool("map_name") 
        || !utils::IsFileExist(GetParamsHolder().GetParam<std::string>("map_name")))
    {
        auto newmob = GetItemFabric()->newItemOnMap<IMob>(
                hash("ork"), 
                GetMapMaster()->squares[GetMapMaster()->GetMapW() / 2]
                                       [GetMapMaster()->GetMapH() / 2]
                                       [GetMapMaster()->GetMapD() / 2]);
        ChangeMob(newmob);
        GetItemFabric()->SetPlayerId(newmob.ret_id(), newmob.ret_id());

        auto tptr = GetItemFabric()->newItemOnMap<IOnMapObject>(
                hash("Teleportator"), 
                GetMapMaster()->squares[GetMapMaster()->GetMapW() / 2]
                                       [GetMapMaster()->GetMapH() / 2]
                                       [GetMapMaster()->GetMapD() / 2]);
        SetCreator(tptr.ret_id());

        srand(SDL_GetTicks());
        GetMapMaster()->makeMap();
    }
    else
    {
       std::string str = GetParamsHolder().GetParam<std::string>("map_name");
       GetItemFabric()->loadMap(str.c_str());
    }
    LiquidHolder::LoadReaction();

    LoginData data;
    data.who = GetMob().ret_id();
    data.word_for_who = 1;
    NetClient::GetNetClient()->Connect(adrs_, DEFAULT_PORT, data);

    Chat::InitChat(sizeW, 0, sizeW + guiShift, sizeH, 32);

    GetTexts()["FPS"].SetUpdater
    ([this](std::string* str)
    {
        std::stringstream ss; 
        ss << last_fps; 
        ss >> *str;
    }).SetFreq(1000).SetSize(20);

    GetTexts()["LastTouch"].SetUpdater
    ([this](std::string* str)
    {
        *str = last_touch;
    }).SetFreq(20).SetPlace(0, 485).SetSize(22);

    
};

void Manager::loadIniFile()
{

};

void Manager::process_in_msg()
{
    Message msg;
    while (true)
    {
        NetClient::GetNetClient()->Recv(&msg);
        if (msg.text == "SDLK_F2")
        {
            SYSTEM_STREAM << "Ping is: " << (SDL_GetTicks() - ping_send) / 1000.0 << "s" << std::endl;
            continue;
        }
        if (msg.text == Net::NEXTTICK)
            return;
        
        id_ptr_on<IMessageReceiver> i;
        i = msg.to;

        if (i.valid())
            i->processGUImsg(msg);
        else
            SYSTEM_STREAM << "Wrong id accepted - " << msg.to << std::endl;
    }
}

bool Manager::isMobVisible(int posx, int posy)
{
    // TODO: matrix for fast check
    if (visiblePoint == nullptr)
        return false;
    for (auto it = visiblePoint->begin(); it != visiblePoint->end(); ++it)
        if(it->posx == posx && it->posy == posy)
            return true;
    return false;
}

Manager* manager_ = nullptr;
Manager* GetManager()
{
    return manager_;
}

void SetManager(Manager* manager)
{
    manager_ = manager;
}


