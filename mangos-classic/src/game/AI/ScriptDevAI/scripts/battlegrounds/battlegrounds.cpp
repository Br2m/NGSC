/* This file is part of the ScriptDev2 Project. See AUTHORS file for Copyright information
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/* ScriptData
SDName: Battleground
SD%Complete: 100
SDComment: Spirit guides in battlegrounds will revive all players every 30 sec
SDCategory: Battlegrounds
EndScriptData

*/

#include "AI/ScriptDevAI/PreCompiledHeader.h"
#include "AI/ScriptDevAI/base/escort_ai.h"

// **** Script Info ****
// Spiritguides in battlegrounds resurrecting many players at once
// every 30 seconds - through a channeled spell, which gets autocasted
// the whole time
// if spiritguide despawns all players around him will get teleported
// to the next spiritguide
// here i'm not sure, if a dummyspell exist for it

// **** Quick Info ****
// battleground spiritguides - this script handles gossipHello
// and JustDied also it let autocast the channel-spell

enum
{
    SPELL_SPIRIT_HEAL_CHANNEL       = 22011,                // Spirit Heal Channel

    SPELL_SPIRIT_HEAL               = 22012,                // Spirit Heal

    SPELL_WAITING_TO_RESURRECT      = 2584                  // players who cancel this aura don't want a resurrection
};

struct npc_spirit_guideAI : public ScriptedAI
{
    npc_spirit_guideAI(Creature* pCreature) : ScriptedAI(pCreature)
    {
        pCreature->SetActiveObjectState(true);
        Reset();
    }

    void Reset() override {}

    void UpdateAI(const uint32 /*uiDiff*/) override
    {
        // auto cast the whole time this spell
        if (!m_creature->GetCurrentSpell(CURRENT_CHANNELED_SPELL))
            m_creature->CastSpell(m_creature, SPELL_SPIRIT_HEAL_CHANNEL, TRIGGERED_NONE);
    }

    void CorpseRemoved(uint32&) override
    {
        // TODO: would be better to cast a dummy spell
        Map* pMap = m_creature->GetMap();

        if (!pMap || !pMap->IsBattleGround())
            return;

        Map::PlayerList const& PlayerList = pMap->GetPlayers();

        for (Map::PlayerList::const_iterator itr = PlayerList.begin(); itr != PlayerList.end(); ++itr)
        {
            Player* pPlayer = itr->getSource();
            if (!pPlayer || !pPlayer->IsWithinDistInMap(m_creature, 20.0f) || !pPlayer->HasAura(SPELL_WAITING_TO_RESURRECT))
                continue;

            // repop player again - now this node won't be counted and another node is searched
            pPlayer->RepopAtGraveyard();
        }
    }
};

bool GossipHello_npc_spirit_guide(Player* pPlayer, Creature* /*pCreature*/)
{
    pPlayer->CastSpell(pPlayer, SPELL_WAITING_TO_RESURRECT, TRIGGERED_OLD_TRIGGERED);
    return true;
}

CreatureAI* GetAI_npc_spirit_guide(Creature* pCreature)
{
    return new npc_spirit_guideAI(pCreature);
}

enum
{
	NPC_SLIDORE_GRYPHON			= 14946,

	SPELL_REND					= 16509,
	SPELL_STRIKE				= 15580,
	SPELL_CONJURE_SLIDORE_BEACON= 21732,

	ITEM_SLIDORE_BEACON			= 17507,

	SAY_SLIDORE_START_ESCORT	= -1134381,
	SAY_SLIDORE_FLIGHT_READY	= -1134382,
	SAY_SLIDORE_START_PATROL	= -1134383,

	GOSSIP_ID_SLIDORE1			= 5148,
	GOSSIP_ID_SLIDORE2			= 5248,

	GOSSIP_ITEM_SLIDORE_ESCORT	= -3134381,
	GOSSIP_ITEM_SLIDORE_BEACON	= -3134382,
	GOSSIP_ITEM_SLIDORE_FLIGHT	= -3134383
};

struct npc_wing_commander_slidoreAI : public npc_escortAI
{
	// CreatureAI functions
	npc_wing_commander_slidoreAI(Creature* pCreature) : npc_escortAI(pCreature)
	{
		Reset();
		m_creature->SetStandState(UNIT_STAND_STATE_DEAD);
		m_creature->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
	}

	uint32 m_uiRendTimer;
	uint32 m_uiStrikeTimer;
	
	uint32 m_uiIntroTimer;
	uint32 m_uiIntroNum;

	uint32 m_uiEndTimer;
	uint32 m_uiEndNum;

	uint32 m_uiPatrolTimer;
	uint32 m_uiPatrolNum;

	// Is called after each combat, so usally only reset combat-stuff here
	void Reset() override
	{
		m_uiRendTimer = 11000;
		m_uiStrikeTimer = 5000;
	}
	// Called when Escort is started.
	void StartEscort()
	{
		m_uiIntroTimer = 1000;
		m_uiIntroNum = 0;
	}
	// Called when Gossip 3 is clicked
	void StartPatrol()
	{
		m_uiPatrolTimer = 1000;
		m_uiPatrolNum = 0;
	}
	// Pure Virtual Functions (Have to be implemented)
	void WaypointReached(uint32 uiWP) override
	{
		if (uiWP == 108)
		{
			SetEscortPaused(true);
			m_uiEndTimer = 1000;
			m_uiEndNum = 0;
		}
	}

	void ReceiveAIEvent(AIEventType eventType, Creature* /*pSender*/, Unit* pInvoker, uint32 uiMiscValue) override
	{
		if (eventType == AI_EVENT_CUSTOM_EVENTAI_A)
		{
			m_creature->RemoveFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_QUESTGIVER);
			m_creature->SetFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_GOSSIP);
			DoScriptText(SAY_SLIDORE_FLIGHT_READY, m_creature);
		}
	}

	void UpdateEscortAI(const uint32 uiDiff) override
	{
		// Escort start
		if (m_uiIntroTimer)
		{
			if (m_uiIntroTimer <= uiDiff)
			{
				switch (m_uiIntroNum)
				{
				case 0:
					m_creature->SetStandState(UNIT_STAND_STATE_STAND);
					m_creature->RemoveFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_GOSSIP);
					m_uiIntroTimer = 2000;
					++m_uiIntroNum;
					break;
				case 1:
					DoScriptText(SAY_SLIDORE_START_ESCORT, m_creature);
					m_uiIntroTimer = 2000;
					++m_uiIntroNum;
					break;
				case 2:
					m_creature->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
					Start(true, nullptr, nullptr, false, false);
					m_uiIntroTimer = 0;
					break;
				}
			}
			else
				m_uiIntroTimer -= uiDiff;
		}
		// Escort End
		if (m_uiEndTimer)
		{
			if (m_uiEndTimer <= uiDiff)
			{
				switch (m_uiEndNum)
				{
				case 0:
					m_creature->GetMotionMaster()->MoveIdle();
					m_uiEndTimer = 2000;
					++m_uiEndNum;
					break;
				case 1:
					m_creature->SetFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_QUESTGIVER);
					m_creature->SetFacingTo(4.46f);
					m_uiEndTimer = 0;
					break;
				}
			}
			else
				m_uiEndTimer -= uiDiff;
		}
		// Patrol start
		if (m_uiPatrolTimer)
		{
			if (m_uiPatrolTimer <= uiDiff)
			{
				switch (m_uiPatrolNum)
				{
				case 0:
					m_creature->RemoveFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_GOSSIP);
					m_uiPatrolTimer = 2000;
					++m_uiPatrolNum;
					break;
				case 1:
					DoScriptText(SAY_SLIDORE_START_PATROL, m_creature);
					m_uiPatrolTimer = 2000;
					++m_uiPatrolNum;
					break;
				case 2:
					m_creature->Mount(1148);
					m_creature->SetLevitate(true);
					m_uiPatrolTimer = 2000;
					++m_uiPatrolNum;
					break;
				case 3:
					m_creature->MonsterMoveWithSpeed(-1074.03f, -324.18f, 200.00f, 20, false, true);
					m_uiPatrolTimer = 10000;
					++m_uiPatrolNum;
					break;
				case 4:
					m_creature->SummonCreature(NPC_SLIDORE_GRYPHON, -1074.03f, -324.18f, 96.7436f, 3.65916f, TEMPSPAWN_MANUAL_DESPAWN, 0, false, false, 1);
					m_creature->ForcedDespawn(2);
					m_uiPatrolTimer = 0;
					break;
				}
			}
			else
				m_uiPatrolTimer -= uiDiff;
		}

		// Combat check
		if (m_creature->SelectHostileTarget() && m_creature->getVictim())
		{
			if (m_uiRendTimer < uiDiff)
			{
				m_creature->CastSpell(m_creature->getVictim(), SPELL_REND, TRIGGERED_NONE);
				m_uiRendTimer = 11000;
			}
			else
				m_uiRendTimer -= uiDiff;

			if (m_uiStrikeTimer < uiDiff)
			{
				m_creature->CastSpell(m_creature->getVictim(), SPELL_STRIKE, TRIGGERED_NONE);
				m_uiStrikeTimer = 5000;
			}
			else
				m_uiStrikeTimer -= uiDiff;

			DoMeleeAttackIfReady();
		}
	}
};

CreatureAI* GetAI_npc_wing_commander_slidore(Creature* pCreature)
{
	return new npc_wing_commander_slidoreAI(pCreature);
}

bool GossipHello_npc_wing_commander_slidore(Player* pPlayer, Creature* pCreature)
{
	pPlayer->TalkedToCreature(pCreature->GetEntry(), pCreature->GetObjectGuid());
	if (pCreature->GetMotionMaster()->getLastReachedWaypoint() < 10)
	{
		pPlayer->PrepareGossipMenu(pCreature, GOSSIP_ID_SLIDORE1);

		if (npc_wing_commander_slidoreAI* pEscortAI = dynamic_cast<npc_wing_commander_slidoreAI*>(pCreature->AI()))
			pPlayer->ADD_GOSSIP_ITEM_ID(GOSSIP_ICON_CHAT, GOSSIP_ITEM_SLIDORE_ESCORT, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 1);

		pPlayer->SendPreparedGossip(pCreature);
	}
	else if (pCreature->GetMotionMaster()->getLastReachedWaypoint() > 10 && pCreature->HasFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_GOSSIP))
	{
		pPlayer->PrepareGossipMenu(pCreature, GOSSIP_ID_SLIDORE2);

		if (pPlayer->GetReputationRank(730) > 5)
		{
			if (!pPlayer->HasItemCount(ITEM_SLIDORE_BEACON, 1, true))
			{
				pPlayer->ADD_GOSSIP_ITEM_ID(GOSSIP_ICON_CHAT, GOSSIP_ITEM_SLIDORE_BEACON, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 2);
			}
			pPlayer->ADD_GOSSIP_ITEM_ID(GOSSIP_ICON_CHAT, GOSSIP_ITEM_SLIDORE_FLIGHT, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 3);
		}
		pPlayer->SendPreparedGossip(pCreature);
	}
	else if (pCreature->GetMotionMaster()->getLastReachedWaypoint() > 1 && pCreature->HasFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_QUESTGIVER))
	{
		pPlayer->PrepareQuestMenu(pCreature->GetGUID());
		pPlayer->SendPreparedQuest(pCreature->GetGUID());
	}
	return true;
}

bool GossipSelect_npc_wing_commander_slidore(Player* pPlayer, Creature* pCreature, uint32 /*uiSender*/, uint32 uiAction)
{
	npc_wing_commander_slidoreAI* pEscortAI = dynamic_cast<npc_wing_commander_slidoreAI*>(pCreature->AI());
	switch (uiAction)
	{
	case GOSSIP_ACTION_INFO_DEF + 1:
		pPlayer->CLOSE_GOSSIP_MENU();
		pEscortAI->StartEscort();
		break;
	case GOSSIP_ACTION_INFO_DEF + 2:
		pPlayer->CLOSE_GOSSIP_MENU();
		pCreature->CastSpell(pPlayer, SPELL_CONJURE_SLIDORE_BEACON, TRIGGERED_NONE);
		break;
	case GOSSIP_ACTION_INFO_DEF + 3:
		pPlayer->CLOSE_GOSSIP_MENU();
		pEscortAI->StartPatrol();
		break;
	default:
		return false;
	}
	return true;                                            // no default handling  -> prevent mangos core handling	
}

enum
{
	NPC_VIPORE_GRYPHON			= 14948,

	SPELL_CONJURE_VIPORE_BEACON = 21734,
	SPELL_VIPORE_CAT_FORM		= 21653,

	ITEM_VIPORE_BEACON			= 17506,

	SAY_VIPORE_START_ESCORT		= -1134391,
	SAY_VIPORE_FLIGHT_READY		= -1134392,
	SAY_VIPORE_START_PATROL		= -1134393,

	GOSSIP_ID_VIPORE1			= 5147,
	GOSSIP_ID_VIPORE2			= 5247,

	GOSSIP_ITEM_VIPORE_ESCORT	= -3134391,
	GOSSIP_ITEM_VIPORE_BEACON	= -3134392,
	GOSSIP_ITEM_VIPORE_FLIGHT	= -3134393
};

struct npc_wing_commander_viporeAI : public npc_escortAI
{
	// CreatureAI functions
	npc_wing_commander_viporeAI(Creature* pCreature) : npc_escortAI(pCreature)
	{
		Reset();
		m_creature->SetStandState(UNIT_STAND_STATE_SIT);
		m_creature->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
	}

	uint32 m_uiRendTimer;
	uint32 m_uiStrikeTimer;

	uint32 m_uiIntroTimer;
	uint32 m_uiIntroNum;

	uint32 m_uiEndTimer;
	uint32 m_uiEndNum;

	uint32 m_uiPatrolTimer;
	uint32 m_uiPatrolNum;

	// Is called after each combat, so usally only reset combat-stuff here
	void Reset() override
	{
		m_uiRendTimer = 11000;
		m_uiStrikeTimer = 5000;
		if (HasEscortState(STATE_ESCORT_ESCORTING))
		{
			m_creature->CastSpell(m_creature, SPELL_VIPORE_CAT_FORM, TRIGGERED_NONE);
		}

	}
	// Called when Escort is started.
	void StartEscort()
	{
		m_uiIntroTimer = 1000;
		m_uiIntroNum = 0;
	}
	// Called when Gossip 3 is clicked
	void StartPatrol()
	{
		m_uiPatrolTimer = 1000;
		m_uiPatrolNum = 0;
	}
	// Pure Virtual Functions (Have to be implemented)
	void WaypointReached(uint32 uiWP) override
	{
		if (uiWP == 170)
		{
			SetEscortPaused(true);
			m_uiEndTimer = 1000;
			m_uiEndNum = 0;
		}
	}

	void ReceiveAIEvent(AIEventType eventType, Creature* /*pSender*/, Unit* pInvoker, uint32 uiMiscValue) override
	{
		if (eventType == AI_EVENT_CUSTOM_EVENTAI_A)
		{
			m_creature->RemoveFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_QUESTGIVER);
			m_creature->SetFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_GOSSIP);
			DoScriptText(SAY_VIPORE_FLIGHT_READY, m_creature);
		}
	}

	void UpdateEscortAI(const uint32 uiDiff) override
	{
		// Escort start
		if (m_uiIntroTimer)
		{
			if (m_uiIntroTimer <= uiDiff)
			{
				switch (m_uiIntroNum)
				{
				case 0:
					m_creature->SetStandState(UNIT_STAND_STATE_STAND);
					m_creature->RemoveFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_GOSSIP);
					m_uiIntroTimer = 2000;
					++m_uiIntroNum;
					break;
				case 1:
					DoScriptText(SAY_VIPORE_START_ESCORT, m_creature);
					m_uiIntroTimer = 2000;
					++m_uiIntroNum;
					break;
				case 2:
					m_creature->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
					m_creature->CastSpell(m_creature, SPELL_VIPORE_CAT_FORM, TRIGGERED_NONE);
					Start(true, nullptr, nullptr, false, false);
					m_uiIntroTimer = 0;
					break;
				}
			}
			else
				m_uiIntroTimer -= uiDiff;
		}
		// Escort End
		if (m_uiEndTimer)
		{
			if (m_uiEndTimer <= uiDiff)
			{
				switch (m_uiEndNum)
				{
				case 0:
					m_creature->RemoveAurasDueToSpell(SPELL_VIPORE_CAT_FORM);
					m_creature->GetMotionMaster()->MoveIdle();
					m_uiEndTimer = 2000;
					++m_uiEndNum;
					break;
				case 1:
					m_creature->SetFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_QUESTGIVER);
					m_creature->SetFacingTo(4.46f);
					m_uiEndTimer = 0;
					break;
				}
			}
			else
				m_uiEndTimer -= uiDiff;
		}
		// Patrol start
		if (m_uiPatrolTimer)
		{
			if (m_uiPatrolTimer <= uiDiff)
			{
				switch (m_uiPatrolNum)
				{
				case 0:
					m_creature->RemoveFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_GOSSIP);
					m_uiPatrolTimer = 2000;
					++m_uiPatrolNum;
					break;
				case 1:
					DoScriptText(SAY_VIPORE_START_PATROL, m_creature);
					m_uiPatrolTimer = 2000;
					++m_uiPatrolNum;
					break;
				case 2:
					m_creature->Mount(1148);
					m_creature->SetLevitate(true);
					m_uiPatrolTimer = 2000;
					++m_uiPatrolNum;
					break;
				case 3:
					m_creature->MonsterMoveWithSpeed(-1074.03f, -324.18f, 500.00f, 20, false, true);
					m_uiPatrolTimer = 10000;
					++m_uiPatrolNum;
					break;
				case 4:
					m_creature->SummonCreature(NPC_VIPORE_GRYPHON, -1248.62f, -348.403f, 89.4532f, 5.04068f, TEMPSPAWN_MANUAL_DESPAWN, 0, false, false, 1);
					m_creature->ForcedDespawn(2);
					m_uiPatrolTimer = 0;
					break;
				}
			}
			else
				m_uiPatrolTimer -= uiDiff;
		}

		// Combat check
		if (m_creature->SelectHostileTarget() && m_creature->getVictim())
		{
			if (m_uiRendTimer < uiDiff)
			{
				m_creature->CastSpell(m_creature->getVictim(), SPELL_REND, TRIGGERED_NONE);
				m_uiRendTimer = 11000;
			}
			else
				m_uiRendTimer -= uiDiff;

			if (m_uiStrikeTimer < uiDiff)
			{
				m_creature->CastSpell(m_creature->getVictim(), SPELL_STRIKE, TRIGGERED_NONE);
				m_uiStrikeTimer = 5000;
			}
			else
				m_uiStrikeTimer -= uiDiff;

			DoMeleeAttackIfReady();
		}
	}
};

CreatureAI* GetAI_npc_wing_commander_vipore(Creature* pCreature)
{
	return new npc_wing_commander_viporeAI(pCreature);
}

bool GossipHello_npc_wing_commander_vipore(Player* pPlayer, Creature* pCreature)
{
	pPlayer->TalkedToCreature(pCreature->GetEntry(), pCreature->GetObjectGuid());
	if (pCreature->GetMotionMaster()->getLastReachedWaypoint() < 10)
	{
		pPlayer->PrepareGossipMenu(pCreature, GOSSIP_ID_VIPORE1);

		if (npc_wing_commander_viporeAI* pEscortAI = dynamic_cast<npc_wing_commander_viporeAI*>(pCreature->AI()))
			pPlayer->ADD_GOSSIP_ITEM_ID(GOSSIP_ICON_CHAT, GOSSIP_ITEM_VIPORE_ESCORT, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 1);

		pPlayer->SendPreparedGossip(pCreature);
	}
	else if (pCreature->GetMotionMaster()->getLastReachedWaypoint() > 10 && pCreature->HasFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_GOSSIP))
	{
		pPlayer->PrepareGossipMenu(pCreature, GOSSIP_ID_VIPORE2);

		if (pPlayer->GetReputationRank(730) > 5)
		{
			if (!pPlayer->HasItemCount(ITEM_VIPORE_BEACON, 1, true))
			{
				pPlayer->ADD_GOSSIP_ITEM_ID(GOSSIP_ICON_CHAT, GOSSIP_ITEM_VIPORE_BEACON, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 2);
			}
			pPlayer->ADD_GOSSIP_ITEM_ID(GOSSIP_ICON_CHAT, GOSSIP_ITEM_VIPORE_FLIGHT, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 3);
		}
		pPlayer->SendPreparedGossip(pCreature);
	}
	else if (pCreature->GetMotionMaster()->getLastReachedWaypoint() > 1 && pCreature->HasFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_QUESTGIVER))
	{
		pPlayer->PrepareQuestMenu(pCreature->GetGUID());
		pPlayer->SendPreparedQuest(pCreature->GetGUID());
	}
	return true;
}

bool GossipSelect_npc_wing_commander_vipore(Player* pPlayer, Creature* pCreature, uint32 /*uiSender*/, uint32 uiAction)
{
	npc_wing_commander_viporeAI* pEscortAI = dynamic_cast<npc_wing_commander_viporeAI*>(pCreature->AI());
	switch (uiAction)
	{
	case GOSSIP_ACTION_INFO_DEF + 1:
		pPlayer->CLOSE_GOSSIP_MENU();
		pEscortAI->StartEscort();
		break;
	case GOSSIP_ACTION_INFO_DEF + 2:
		pPlayer->CLOSE_GOSSIP_MENU();
		pCreature->CastSpell(pPlayer, SPELL_CONJURE_VIPORE_BEACON, TRIGGERED_NONE);
		break;
	case GOSSIP_ACTION_INFO_DEF + 3:
		pPlayer->CLOSE_GOSSIP_MENU();
		pEscortAI->StartPatrol();
		break;
	default:
		return false;
	}
	return true;                                            // no default handling  -> prevent mangos core handling	
}

enum
{
	NPC_ICHMAN_GRYPHON			= 14947,

	SPELL_CONJURE_ICHMAN_BEACON = 21731,
	SPELL_INVISIBILITY			= 16380,

	ITEM_ICHMAN_BEACON			= 17505,

	SAY_ICHMAN_START_ESCORT		= -1134371,
	SAY_ICHMAN_TELEPORT			= -1134374,
	SAY_ICHMAN_FLIGHT_READY		= -1134372,
	SAY_ICHMAN_START_PATROL		= -1134373,

	GOSSIP_ID_ICHMAN1			= 5146,
	GOSSIP_ID_ICHMAN2			= 5246,

	GOSSIP_ITEM_ICHMAN_ESCORT	= -3134371,
	GOSSIP_ITEM_ICHMAN_BEACON	= -3134372,
	GOSSIP_ITEM_ICHMAN_FLIGHT	= -3134373
};

struct npc_wing_commander_ichmanAI : public npc_escortAI
{
	// CreatureAI functions
	npc_wing_commander_ichmanAI(Creature* pCreature) : npc_escortAI(pCreature)
	{
		Reset();
		m_creature->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
	}

	uint32 m_uiRendTimer;
	uint32 m_uiStrikeTimer;

	uint32 m_uiIntroTimer;
	uint32 m_uiIntroNum;

	uint32 m_uiEndTimer;
	uint32 m_uiEndNum;

	uint32 m_uiPatrolTimer;
	uint32 m_uiPatrolNum;

	// Is called after each combat, so usally only reset combat-stuff here
	void Reset() override
	{
		m_uiRendTimer = 11000;
		m_uiStrikeTimer = 5000;
	}
	// Called when Escort is started.
	void StartEscort()
	{
		m_uiIntroTimer = 1000;
		m_uiIntroNum = 0;
	}
	// Called when Gossip 3 is clicked
	void StartPatrol()
	{
		m_uiPatrolTimer = 1000;
		m_uiPatrolNum = 0;
	}
	// Pure Virtual Functions (Have to be implemented)
	void WaypointReached(uint32 uiWP) override
	{
		if (uiWP == 115)
		{
			SetEscortPaused(true);
			m_uiEndTimer = 1000;
			m_uiEndNum = 0;
		}
	}

	void ReceiveAIEvent(AIEventType eventType, Creature* /*pSender*/, Unit* pInvoker, uint32 uiMiscValue) override
	{
		if (eventType == AI_EVENT_CUSTOM_EVENTAI_A)
		{
			m_creature->RemoveFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_QUESTGIVER);
			m_creature->SetFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_GOSSIP);
			DoScriptText(SAY_ICHMAN_FLIGHT_READY, m_creature);
		}
	}

	void UpdateEscortAI(const uint32 uiDiff) override
	{
		// Escort start
		if (m_uiIntroTimer)
		{
			if (m_uiIntroTimer <= uiDiff)
			{
				switch (m_uiIntroNum)
				{
				case 0:
					m_creature->RemoveFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_GOSSIP);
					m_uiIntroTimer = 2000;
					++m_uiIntroNum;
					break;
				case 1:
					DoScriptText(SAY_ICHMAN_START_ESCORT, m_creature);
					m_uiIntroTimer = 2000;
					++m_uiIntroNum;
					break;
				case 2:
					m_creature->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
					Start(true, nullptr, nullptr, false, false);
					m_uiIntroTimer = 0;
					break;
				}
			}
			else
				m_uiIntroTimer -= uiDiff;
		}
		// Escort End
		if (m_uiEndTimer)
		{
			if (m_uiEndTimer <= uiDiff)
			{
				switch (m_uiEndNum)
				{
				case 0:
					m_creature->GetMotionMaster()->MoveIdle();
					m_creature->SetFacingTo(2.609f);
					m_uiEndTimer = 2000;
					++m_uiEndNum;
					break;
				case 1:
					DoScriptText(SAY_ICHMAN_TELEPORT, m_creature);
					m_uiEndTimer = 2000;
					++m_uiEndNum;
					break;
				case 2:
					m_creature->CastSpell(m_creature, SPELL_INVISIBILITY, TRIGGERED_NONE);
					m_uiEndTimer = 1000;
					++m_uiEndNum;
					break;
				case 3:
					m_creature->NearTeleportTo(569.826477f, -50.058575f, 38.402222f, 1.166308f, false);
					m_creature->SetFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_QUESTGIVER);					
					m_uiEndTimer = 1000;
					++m_uiEndNum;
					break;
				case 4:
					m_creature->RemoveAurasDueToSpell(SPELL_INVISIBILITY);
					m_uiEndTimer = 0;
					break;
				}
			}
			else
				m_uiEndTimer -= uiDiff;
		}
		// Patrol start
		if (m_uiPatrolTimer)
		{
			if (m_uiPatrolTimer <= uiDiff)
			{
				switch (m_uiPatrolNum)
				{
				case 0:
					m_creature->RemoveFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_GOSSIP);
					m_uiPatrolTimer = 2000;
					++m_uiPatrolNum;
					break;
				case 1:
					DoScriptText(SAY_ICHMAN_START_PATROL, m_creature);
					m_creature->SummonCreature(NPC_ICHMAN_GRYPHON, 520.8f, -43.5716f, 47.01f, 0.110732f, TEMPSPAWN_TIMED_DESPAWN, 10000, false, false, 2);
					m_uiPatrolTimer = 2000;
					++m_uiPatrolNum;
					break;
				case 2:
					m_creature->ForcedDespawn(10);
					m_uiPatrolTimer = 2000;
					++m_uiPatrolNum;
					break;
				case 3:
					m_creature->SummonCreature(NPC_ICHMAN_GRYPHON, -1348.7f, -289.173f, 120.941f, 3.80289f, TEMPSPAWN_MANUAL_DESPAWN, 0, false, false, 1);
					m_uiPatrolTimer = 0;
					break;
				}
			}
			else
				m_uiPatrolTimer -= uiDiff;
		}

		// Combat check
		if (m_creature->SelectHostileTarget() && m_creature->getVictim())
		{
			if (m_uiRendTimer < uiDiff)
			{
				m_creature->CastSpell(m_creature->getVictim(), SPELL_REND, TRIGGERED_NONE);
				m_uiRendTimer = 11000;
			}
			else
				m_uiRendTimer -= uiDiff;

			if (m_uiStrikeTimer < uiDiff)
			{
				m_creature->CastSpell(m_creature->getVictim(), SPELL_STRIKE, TRIGGERED_NONE);
				m_uiStrikeTimer = 5000;
			}
			else
				m_uiStrikeTimer -= uiDiff;

			DoMeleeAttackIfReady();
		}
	}
};

CreatureAI* GetAI_npc_wing_commander_ichman(Creature* pCreature)
{
	return new npc_wing_commander_ichmanAI(pCreature);
}

bool GossipHello_npc_wing_commander_ichman(Player* pPlayer, Creature* pCreature)
{
	pPlayer->TalkedToCreature(pCreature->GetEntry(), pCreature->GetObjectGuid());
	if (pCreature->GetMotionMaster()->getLastReachedWaypoint() < 10)
	{
		pPlayer->PrepareGossipMenu(pCreature, GOSSIP_ID_ICHMAN1);

		if (npc_wing_commander_ichmanAI* pEscortAI = dynamic_cast<npc_wing_commander_ichmanAI*>(pCreature->AI()))
			pPlayer->ADD_GOSSIP_ITEM_ID(GOSSIP_ICON_CHAT, GOSSIP_ITEM_ICHMAN_ESCORT, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 1);

		pPlayer->SendPreparedGossip(pCreature);
	}
	else if (pCreature->GetMotionMaster()->getLastReachedWaypoint() > 10 && pCreature->HasFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_GOSSIP))
	{
		pPlayer->PrepareGossipMenu(pCreature, GOSSIP_ID_ICHMAN2);

		if (pPlayer->GetReputationRank(730) > 5)
		{
			if (!pPlayer->HasItemCount(ITEM_ICHMAN_BEACON, 1, true))
			{
				pPlayer->ADD_GOSSIP_ITEM_ID(GOSSIP_ICON_CHAT, GOSSIP_ITEM_ICHMAN_BEACON, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 2);
			}
			pPlayer->ADD_GOSSIP_ITEM_ID(GOSSIP_ICON_CHAT, GOSSIP_ITEM_ICHMAN_FLIGHT, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 3);
		}
		pPlayer->SendPreparedGossip(pCreature);
	}
	else if (pCreature->GetMotionMaster()->getLastReachedWaypoint() > 1 && pCreature->HasFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_QUESTGIVER))
	{
		pPlayer->PrepareQuestMenu(pCreature->GetGUID());
		pPlayer->SendPreparedQuest(pCreature->GetGUID());
	}
	return true;
}

bool GossipSelect_npc_wing_commander_ichman(Player* pPlayer, Creature* pCreature, uint32 /*uiSender*/, uint32 uiAction)
{
	npc_wing_commander_ichmanAI* pEscortAI = dynamic_cast<npc_wing_commander_ichmanAI*>(pCreature->AI());
	switch (uiAction)
	{
	case GOSSIP_ACTION_INFO_DEF + 1:
		pPlayer->CLOSE_GOSSIP_MENU();
		pEscortAI->StartEscort();
		break;
	case GOSSIP_ACTION_INFO_DEF + 2:
		pPlayer->CLOSE_GOSSIP_MENU();
		pCreature->CastSpell(pPlayer, SPELL_CONJURE_ICHMAN_BEACON, TRIGGERED_NONE);
		break;
	case GOSSIP_ACTION_INFO_DEF + 3:
		pPlayer->CLOSE_GOSSIP_MENU();
		pEscortAI->StartPatrol();
		break;
	default:
		return false;
	}
	return true;                                            // no default handling  -> prevent mangos core handling	
}

enum
{
	NPC_GUSE_WAR_RIDER			= 14943,

	SPELL_CONJURE_GUSE_BEACON	= 21356,

	ITEM_GUSE_BEACON			= 17324,

	SAY_GUSE_START_ESCORT		= -1131791,
	SAY_GUSE_FLIGHT_READY		= -1131792,
	SAY_GUSE_START_PATROL		= -1131793,

	GOSSIP_ID_GUSE1				= 5179,
	GOSSIP_ID_GUSE2				= 5279,

	GOSSIP_ITEM_GUSE_ESCORT		= -3131791,
	GOSSIP_ITEM_GUSE_BEACON		= -3131792,
	GOSSIP_ITEM_GUSE_FLIGHT		= -3131793
};

struct npc_wing_commander_guseAI : public npc_escortAI
{
	// CreatureAI functions
	npc_wing_commander_guseAI(Creature* pCreature) : npc_escortAI(pCreature)
	{
		Reset();
		m_creature->SetStandState(UNIT_STAND_STATE_DEAD);
		m_creature->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
	}

	uint32 m_uiRendTimer;
	uint32 m_uiStrikeTimer;

	uint32 m_uiIntroTimer;
	uint32 m_uiIntroNum;

	uint32 m_uiEndTimer;
	uint32 m_uiEndNum;

	uint32 m_uiPatrolTimer;
	uint32 m_uiPatrolNum;

	// Is called after each combat, so usally only reset combat-stuff here
	void Reset() override
	{
		m_uiRendTimer = 11000;
		m_uiStrikeTimer = 5000;
	}
	// Called when Escort is started.
	void StartEscort()
	{
		m_uiIntroTimer = 1000;
		m_uiIntroNum = 0;
	}
	// Called when Gossip 3 is clicked
	void StartPatrol()
	{
		m_uiPatrolTimer = 1000;
		m_uiPatrolNum = 0;
	}
	// Pure Virtual Functions (Have to be implemented)
	void WaypointReached(uint32 uiWP) override
	{
		if (uiWP == 132)
		{
			SetEscortPaused(true);
			m_uiEndTimer = 1000;
			m_uiEndNum = 0;
		}
	}

	void ReceiveAIEvent(AIEventType eventType, Creature* /*pSender*/, Unit* pInvoker, uint32 uiMiscValue) override
	{
		if (eventType == AI_EVENT_CUSTOM_EVENTAI_A)
		{
			m_creature->RemoveFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_QUESTGIVER);
			m_creature->SetFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_GOSSIP);
			DoScriptText(SAY_GUSE_FLIGHT_READY, m_creature);
		}
	}

	void UpdateEscortAI(const uint32 uiDiff) override
	{
		// Escort start
		if (m_uiIntroTimer)
		{
			if (m_uiIntroTimer <= uiDiff)
			{
				switch (m_uiIntroNum)
				{
				case 0:
					m_creature->SetStandState(UNIT_STAND_STATE_STAND);
					m_creature->RemoveFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_GOSSIP);
					m_uiIntroTimer = 2000;
					++m_uiIntroNum;
					break;
				case 1:
					DoScriptText(SAY_GUSE_START_ESCORT, m_creature);
					m_uiIntroTimer = 2000;
					++m_uiIntroNum;
					break;
				case 2:
					m_creature->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
					Start(true, nullptr, nullptr, false, false);
					m_uiIntroTimer = 0;
					break;
				}
			}
			else
				m_uiIntroTimer -= uiDiff;
		}
		// Escort End
		if (m_uiEndTimer)
		{
			if (m_uiEndTimer <= uiDiff)
			{
				switch (m_uiEndNum)
				{
				case 0:
					m_creature->GetMotionMaster()->MoveIdle();
					m_uiEndTimer = 2000;
					++m_uiEndNum;
					break;
				case 1:
					m_creature->SetFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_QUESTGIVER);
					m_creature->SetFacingTo(2.020818f);
					m_uiEndTimer = 0;
					break;
				}
			}
			else
				m_uiEndTimer -= uiDiff;
		}
		// Patrol start
		if (m_uiPatrolTimer)
		{
			if (m_uiPatrolTimer <= uiDiff)
			{
				switch (m_uiPatrolNum)
				{
				case 0:
					m_creature->RemoveFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_GOSSIP);
					m_uiPatrolTimer = 2000;
					++m_uiPatrolNum;
					break;
				case 1:
					DoScriptText(SAY_GUSE_START_PATROL, m_creature);
					m_uiPatrolTimer = 2000;
					++m_uiPatrolNum;
					break;
				case 2:
					m_creature->Mount(11012);
					m_creature->SetLevitate(true);
					m_uiPatrolTimer = 2000;
					++m_uiPatrolNum;
					break;
				case 3:
					m_creature->MonsterMoveWithSpeed(-1303.630737f, -288.764252f, 500.00f, 40, false, true);
					m_uiPatrolTimer = 10000;
					++m_uiPatrolNum;
					break;
				case 4:
					m_creature->SummonCreature(NPC_GUSE_WAR_RIDER, 670.582f, -283.38f, 83.251f, 2.53132f, TEMPSPAWN_MANUAL_DESPAWN, 0, false, false, 1);
					m_creature->ForcedDespawn(2);
					m_uiPatrolTimer = 0;
					break;
				}
			}
			else
				m_uiPatrolTimer -= uiDiff;
		}

		// Combat check
		if (m_creature->SelectHostileTarget() && m_creature->getVictim())
		{
			if (m_uiRendTimer < uiDiff)
			{
				m_creature->CastSpell(m_creature->getVictim(), SPELL_REND, TRIGGERED_NONE);
				m_uiRendTimer = 11000;
			}
			else
				m_uiRendTimer -= uiDiff;

			if (m_uiStrikeTimer < uiDiff)
			{
				m_creature->CastSpell(m_creature->getVictim(), SPELL_STRIKE, TRIGGERED_NONE);
				m_uiStrikeTimer = 5000;
			}
			else
				m_uiStrikeTimer -= uiDiff;

			DoMeleeAttackIfReady();
		}
	}
};

CreatureAI* GetAI_npc_wing_commander_guse(Creature* pCreature)
{
	return new npc_wing_commander_guseAI(pCreature);
}

bool GossipHello_npc_wing_commander_guse(Player* pPlayer, Creature* pCreature)
{
	pPlayer->TalkedToCreature(pCreature->GetEntry(), pCreature->GetObjectGuid());
	if (pCreature->GetMotionMaster()->getLastReachedWaypoint() < 10)
	{
		pPlayer->PrepareGossipMenu(pCreature, GOSSIP_ID_GUSE1);

		if (npc_wing_commander_guseAI* pEscortAI = dynamic_cast<npc_wing_commander_guseAI*>(pCreature->AI()))
			pPlayer->ADD_GOSSIP_ITEM_ID(GOSSIP_ICON_CHAT, GOSSIP_ITEM_GUSE_ESCORT, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 1);

		pPlayer->SendPreparedGossip(pCreature);
	}
	else if (pCreature->GetMotionMaster()->getLastReachedWaypoint() > 10 && pCreature->HasFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_GOSSIP))
	{
		pPlayer->PrepareGossipMenu(pCreature, GOSSIP_ID_GUSE2);

		if (pPlayer->GetReputationRank(729) > 5)
		{
			if (!pPlayer->HasItemCount(ITEM_GUSE_BEACON, 1, true))
			{
				pPlayer->ADD_GOSSIP_ITEM_ID(GOSSIP_ICON_CHAT, GOSSIP_ITEM_GUSE_BEACON, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 2);
			}
			pPlayer->ADD_GOSSIP_ITEM_ID(GOSSIP_ICON_CHAT, GOSSIP_ITEM_GUSE_FLIGHT, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 3);
		}
		pPlayer->SendPreparedGossip(pCreature);
	}
	else if (pCreature->GetMotionMaster()->getLastReachedWaypoint() > 1 && pCreature->HasFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_QUESTGIVER))
	{
		pPlayer->PrepareQuestMenu(pCreature->GetGUID());
		pPlayer->SendPreparedQuest(pCreature->GetGUID());
	}
	return true;
}

bool GossipSelect_npc_wing_commander_guse(Player* pPlayer, Creature* pCreature, uint32 /*uiSender*/, uint32 uiAction)
{
	npc_wing_commander_guseAI* pEscortAI = dynamic_cast<npc_wing_commander_guseAI*>(pCreature->AI());
	switch (uiAction)
	{
	case GOSSIP_ACTION_INFO_DEF + 1:
		pPlayer->CLOSE_GOSSIP_MENU();
		pEscortAI->StartEscort();
		break;
	case GOSSIP_ACTION_INFO_DEF + 2:
		pPlayer->CLOSE_GOSSIP_MENU();
		pCreature->CastSpell(pPlayer, SPELL_CONJURE_GUSE_BEACON, TRIGGERED_NONE);
		break;
	case GOSSIP_ACTION_INFO_DEF + 3:
		pPlayer->CLOSE_GOSSIP_MENU();
		pEscortAI->StartPatrol();
		break;
	default:
		return false;
	}
	return true;                                            // no default handling  -> prevent mangos core handling	
}

enum
{
	NPC_JEZTOR_WAR_RIDER = 14944,

	SPELL_CONJURE_JEZTOR_BEACON = 21368,

	ITEM_JEZTOR_BEACON = 17325,

	SAY_JEZTOR_START_ESCORT = -1131801,
	SAY_JEZTOR_FLIGHT_READY = -1131802,
	SAY_JEZTOR_START_PATROL = -1131803,

	GOSSIP_ID_JEZTOR1 = 5180,
	GOSSIP_ID_JEZTOR2 = 5280,

	GOSSIP_ITEM_JEZTOR_ESCORT = -3131801,
	GOSSIP_ITEM_JEZTOR_BEACON = -3131802,
	GOSSIP_ITEM_JEZTOR_FLIGHT = -3131803
};

struct npc_wing_commander_jeztorAI : public npc_escortAI
{
	// CreatureAI functions
	npc_wing_commander_jeztorAI(Creature* pCreature) : npc_escortAI(pCreature)
	{
		Reset();
		m_creature->SetStandState(UNIT_STAND_STATE_DEAD);
		m_creature->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
	}

	uint32 m_uiRendTimer;
	uint32 m_uiStrikeTimer;

	uint32 m_uiIntroTimer;
	uint32 m_uiIntroNum;

	uint32 m_uiEndTimer;
	uint32 m_uiEndNum;

	uint32 m_uiPatrolTimer;
	uint32 m_uiPatrolNum;

	// Is called after each combat, so usally only reset combat-stuff here
	void Reset() override
	{
		m_uiRendTimer = 11000;
		m_uiStrikeTimer = 5000;
	}
	// Called when Escort is started.
	void StartEscort()
	{
		m_uiIntroTimer = 1000;
		m_uiIntroNum = 0;
	}
	// Called when Gossip 3 is clicked
	void StartPatrol()
	{
		m_uiPatrolTimer = 1000;
		m_uiPatrolNum = 0;
	}
	// Pure Virtual Functions (Have to be implemented)
	void WaypointReached(uint32 uiWP) override
	{
		if (uiWP == 132)
		{
			SetEscortPaused(true);
			m_uiEndTimer = 1000;
			m_uiEndNum = 0;
		}
	}

	void ReceiveAIEvent(AIEventType eventType, Creature* /*pSender*/, Unit* pInvoker, uint32 uiMiscValue) override
	{
		if (eventType == AI_EVENT_CUSTOM_EVENTAI_A)
		{
			m_creature->RemoveFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_QUESTGIVER);
			m_creature->SetFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_GOSSIP);
			DoScriptText(SAY_JEZTOR_FLIGHT_READY, m_creature);
		}
	}

	void UpdateEscortAI(const uint32 uiDiff) override
	{
		// Escort start
		if (m_uiIntroTimer)
		{
			if (m_uiIntroTimer <= uiDiff)
			{
				switch (m_uiIntroNum)
				{
				case 0:
					m_creature->SetStandState(UNIT_STAND_STATE_STAND);
					m_creature->RemoveFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_GOSSIP);
					m_uiIntroTimer = 2000;
					++m_uiIntroNum;
					break;
				case 1:
					DoScriptText(SAY_JEZTOR_START_ESCORT, m_creature);
					m_uiIntroTimer = 2000;
					++m_uiIntroNum;
					break;
				case 2:
					m_creature->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
					Start(true, nullptr, nullptr, false, false);
					m_uiIntroTimer = 0;
					break;
				}
			}
			else
				m_uiIntroTimer -= uiDiff;
		}
		// Escort End
		if (m_uiEndTimer)
		{
			if (m_uiEndTimer <= uiDiff)
			{
				switch (m_uiEndNum)
				{
				case 0:
					m_creature->GetMotionMaster()->MoveIdle();
					m_uiEndTimer = 2000;
					++m_uiEndNum;
					break;
				case 1:
					m_creature->SetFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_QUESTGIVER);
					m_creature->SetFacingTo(6.243118f);
					m_uiEndTimer = 0;
					break;
				}
			}
			else
				m_uiEndTimer -= uiDiff;
		}
		// Patrol start
		if (m_uiPatrolTimer)
		{
			if (m_uiPatrolTimer <= uiDiff)
			{
				switch (m_uiPatrolNum)
				{
				case 0:
					m_creature->RemoveFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_GOSSIP);
					m_uiPatrolTimer = 2000;
					++m_uiPatrolNum;
					break;
				case 1:
					DoScriptText(SAY_JEZTOR_START_PATROL, m_creature);
					m_uiPatrolTimer = 2000;
					++m_uiPatrolNum;
					break;
				case 2:
					m_creature->Mount(11012);
					m_creature->SetLevitate(true);
					m_uiPatrolTimer = 2000;
					++m_uiPatrolNum;
					break;
				case 3:
					m_creature->MonsterMoveWithSpeed(-1303.630737f, -288.764252f, 500.00f, 40, false, true);
					m_uiPatrolTimer = 10000;
					++m_uiPatrolNum;
					break;
				case 4:
					m_creature->SummonCreature(NPC_JEZTOR_WAR_RIDER, 641.157f, -123.033f, 113.2225f, 1.31867f, TEMPSPAWN_MANUAL_DESPAWN, 0, false, false, 1);
					m_creature->ForcedDespawn(2);
					m_uiPatrolTimer = 0;
					break;
				}
			}
			else
				m_uiPatrolTimer -= uiDiff;
		}

		// Combat check
		if (m_creature->SelectHostileTarget() && m_creature->getVictim())
		{
			if (m_uiRendTimer < uiDiff)
			{
				m_creature->CastSpell(m_creature->getVictim(), SPELL_REND, TRIGGERED_NONE);
				m_uiRendTimer = 11000;
			}
			else
				m_uiRendTimer -= uiDiff;

			if (m_uiStrikeTimer < uiDiff)
			{
				m_creature->CastSpell(m_creature->getVictim(), SPELL_STRIKE, TRIGGERED_NONE);
				m_uiStrikeTimer = 5000;
			}
			else
				m_uiStrikeTimer -= uiDiff;

			DoMeleeAttackIfReady();
		}
	}
};

CreatureAI* GetAI_npc_wing_commander_jeztor(Creature* pCreature)
{
	return new npc_wing_commander_jeztorAI(pCreature);
}

bool GossipHello_npc_wing_commander_jeztor(Player* pPlayer, Creature* pCreature)
{
	pPlayer->TalkedToCreature(pCreature->GetEntry(), pCreature->GetObjectGuid());
	if (pCreature->GetMotionMaster()->getLastReachedWaypoint() < 10)
	{
		pPlayer->PrepareGossipMenu(pCreature, GOSSIP_ID_JEZTOR1);

		if (npc_wing_commander_jeztorAI* pEscortAI = dynamic_cast<npc_wing_commander_jeztorAI*>(pCreature->AI()))
			pPlayer->ADD_GOSSIP_ITEM_ID(GOSSIP_ICON_CHAT, GOSSIP_ITEM_JEZTOR_ESCORT, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 1);

		pPlayer->SendPreparedGossip(pCreature);
	}
	else if (pCreature->GetMotionMaster()->getLastReachedWaypoint() > 10 && pCreature->HasFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_GOSSIP))
	{
		pPlayer->PrepareGossipMenu(pCreature, GOSSIP_ID_JEZTOR2);

		if (pPlayer->GetReputationRank(729) > 5)
		{
			if (!pPlayer->HasItemCount(ITEM_JEZTOR_BEACON, 1, true))
			{
				pPlayer->ADD_GOSSIP_ITEM_ID(GOSSIP_ICON_CHAT, GOSSIP_ITEM_JEZTOR_BEACON, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 2);
			}
			pPlayer->ADD_GOSSIP_ITEM_ID(GOSSIP_ICON_CHAT, GOSSIP_ITEM_JEZTOR_FLIGHT, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 3);
		}
		pPlayer->SendPreparedGossip(pCreature);
	}
	else if (pCreature->GetMotionMaster()->getLastReachedWaypoint() > 1 && pCreature->HasFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_QUESTGIVER))
	{
		pPlayer->PrepareQuestMenu(pCreature->GetGUID());
		pPlayer->SendPreparedQuest(pCreature->GetGUID());
	}
	return true;
}

bool GossipSelect_npc_wing_commander_jeztor(Player* pPlayer, Creature* pCreature, uint32 /*uiSender*/, uint32 uiAction)
{
	npc_wing_commander_jeztorAI* pEscortAI = dynamic_cast<npc_wing_commander_jeztorAI*>(pCreature->AI());
	switch (uiAction)
	{
	case GOSSIP_ACTION_INFO_DEF + 1:
		pPlayer->CLOSE_GOSSIP_MENU();
		pEscortAI->StartEscort();
		break;
	case GOSSIP_ACTION_INFO_DEF + 2:
		pPlayer->CLOSE_GOSSIP_MENU();
		pCreature->CastSpell(pPlayer, SPELL_CONJURE_JEZTOR_BEACON, TRIGGERED_NONE);
		break;
	case GOSSIP_ACTION_INFO_DEF + 3:
		pPlayer->CLOSE_GOSSIP_MENU();
		pEscortAI->StartPatrol();
		break;
	default:
		return false;
	}
	return true;                                            // no default handling  -> prevent mangos core handling	
}

enum
{
	NPC_MULVERICK_WAR_RIDER = 14945,

	SPELL_CONJURE_MULVERICK_BEACON = 21372,

	ITEM_MULVERICK_BEACON = 17323,

	SAY_MULVERICK_START_ESCORT = -1131811,
	SAY_MULVERICK_FLIGHT_READY = -1131812,
	SAY_MULVERICK_START_PATROL = -1131813,
	SAY_MULVERICK_ESCORT_COMING= -1131814,
	SAY_MULVERICK_START_ESCORT2= -1131815,

	GOSSIP_ID_MULVERICK1 = 5181,
	GOSSIP_ID_MULVERICK2 = 5281,

	GOSSIP_ITEM_MULVERICK_ESCORT = -3131811,
	GOSSIP_ITEM_MULVERICK_BEACON = -3131812,
	GOSSIP_ITEM_MULVERICK_FLIGHT = -3131813
};

struct npc_wing_commander_mulverickAI : public npc_escortAI
{
	// CreatureAI functions
	npc_wing_commander_mulverickAI(Creature* pCreature) : npc_escortAI(pCreature)
	{
		Reset();
		m_creature->SetStandState(UNIT_STAND_STATE_DEAD);
		m_creature->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
	}

	uint32 m_uiRendTimer;
	uint32 m_uiStrikeTimer;

	uint32 m_uiIntroTimer;
	uint32 m_uiIntroNum;

	uint32 m_uiYellTimer;
	uint32 m_uiYellNum;

	uint32 m_uiEndTimer;
	uint32 m_uiEndNum;

	uint32 m_uiPatrolTimer;
	uint32 m_uiPatrolNum;

	// Is called after each combat, so usally only reset combat-stuff here
	void Reset() override
	{
		m_uiRendTimer = 11000;
		m_uiStrikeTimer = 5000;
	}
	// Called when Escort is started.
	void StartEscort()
	{
		m_uiIntroTimer = 1000;
		m_uiIntroNum = 0;
	}
	// Called when Gossip 3 is clicked
	void StartPatrol()
	{
		m_uiPatrolTimer = 1000;
		m_uiPatrolNum = 0;
	}
	// Pure Virtual Functions (Have to be implemented)
	void WaypointReached(uint32 uiWP) override
	{
		switch (uiWP)
		{
		case 74:
			SetEscortPaused(true);
			m_uiYellTimer = 1000;
			m_uiYellNum = 0;
			break;
		case 171:
			SetEscortPaused(true);
			m_uiEndTimer = 1000;
			m_uiEndNum = 0;
			break;
		}
	}

	void ReceiveAIEvent(AIEventType eventType, Creature* /*pSender*/, Unit* pInvoker, uint32 uiMiscValue) override
	{
		if (eventType == AI_EVENT_CUSTOM_EVENTAI_A)
		{
			m_creature->RemoveFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_QUESTGIVER);
			m_creature->SetFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_GOSSIP);
			DoScriptText(SAY_MULVERICK_FLIGHT_READY, m_creature);
		}
	}

	void UpdateEscortAI(const uint32 uiDiff) override
	{
		// Escort start
		if (m_uiIntroTimer)
		{
			if (m_uiIntroTimer <= uiDiff)
			{
				switch (m_uiIntroNum)
				{
				case 0:
					m_creature->SetStandState(UNIT_STAND_STATE_STAND);
					m_creature->RemoveFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_GOSSIP);
					m_uiIntroTimer = 2000;
					++m_uiIntroNum;
					break;
				case 1:
					DoScriptText(SAY_MULVERICK_START_ESCORT, m_creature);
					m_uiIntroTimer = 2000;
					++m_uiIntroNum;
					break;
				case 2:
					DoScriptText(SAY_MULVERICK_START_ESCORT2, m_creature);
					m_uiIntroTimer = 2000;
					++m_uiIntroNum;
					break;
				case 3:
					Start(true, nullptr, nullptr, false, false);
					m_creature->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
					m_uiIntroTimer = 0;
					break;
				}
			}
			else
				m_uiIntroTimer -= uiDiff;
		}
		// Yells in escort
		if (m_uiYellTimer)
		{
			if (m_uiYellTimer <= uiDiff)
			{
				switch (m_uiYellNum)
				{
				case 0:
					DoScriptText(SAY_MULVERICK_ESCORT_COMING, m_creature);
					m_uiYellTimer = 2000;
					++m_uiYellNum;
					break;
				case 1:
					SetEscortPaused(false);
					m_uiYellTimer = 0;
					break;
				}
			}
			else
				m_uiYellTimer -= uiDiff;
		}
		// Escort End
		if (m_uiEndTimer)
		{
			if (m_uiEndTimer <= uiDiff)
			{
				switch (m_uiEndNum)
				{
				case 0:
					m_creature->GetMotionMaster()->MoveIdle();
					m_uiEndTimer = 2000;
					++m_uiEndNum;
					break;
				case 1:
					m_creature->SetFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_QUESTGIVER);
					m_creature->SetFacingTo(0.798738f);
					m_uiEndTimer = 0;
					break;
				}
			}
			else
				m_uiEndTimer -= uiDiff;
		}
		// Patrol start
		if (m_uiPatrolTimer)
		{
			if (m_uiPatrolTimer <= uiDiff)
			{
				switch (m_uiPatrolNum)
				{
				case 0:
					m_creature->RemoveFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_GOSSIP);
					m_uiPatrolTimer = 2000;
					++m_uiPatrolNum;
					break;
				case 1:
					DoScriptText(SAY_MULVERICK_START_PATROL, m_creature);
					m_uiPatrolTimer = 2000;
					++m_uiPatrolNum;
					break;
				case 2:
					m_creature->Mount(11012);
					m_creature->SetLevitate(true);
					m_uiPatrolTimer = 2000;
					++m_uiPatrolNum;
					break;
				case 3:
					m_creature->MonsterMoveWithSpeed(-1303.630737f, -288.764252f, 500.00f, 40, false, true);
					m_uiPatrolTimer = 10000;
					++m_uiPatrolNum;
					break;
				case 4:
					m_creature->SummonCreature(NPC_MULVERICK_WAR_RIDER, 605.494f, -39.2881f, 105.7036f, 3.5547f, TEMPSPAWN_MANUAL_DESPAWN, 0, false, false, 1);
					m_creature->ForcedDespawn(2);
					m_uiPatrolTimer = 0;
					break;
				}
			}
			else
				m_uiPatrolTimer -= uiDiff;
		}

		// Combat check
		if (m_creature->SelectHostileTarget() && m_creature->getVictim())
		{
			if (m_uiRendTimer < uiDiff)
			{
				m_creature->CastSpell(m_creature->getVictim(), SPELL_REND, TRIGGERED_NONE);
				m_uiRendTimer = 11000;
			}
			else
				m_uiRendTimer -= uiDiff;

			if (m_uiStrikeTimer < uiDiff)
			{
				m_creature->CastSpell(m_creature->getVictim(), SPELL_STRIKE, TRIGGERED_NONE);
				m_uiStrikeTimer = 5000;
			}
			else
				m_uiStrikeTimer -= uiDiff;

			DoMeleeAttackIfReady();
		}
	}
};

CreatureAI* GetAI_npc_wing_commander_mulverick(Creature* pCreature)
{
	return new npc_wing_commander_mulverickAI(pCreature);
}

bool GossipHello_npc_wing_commander_mulverick(Player* pPlayer, Creature* pCreature)
{
	pPlayer->TalkedToCreature(pCreature->GetEntry(), pCreature->GetObjectGuid());
	if (pCreature->GetMotionMaster()->getLastReachedWaypoint() < 10)
	{
		pPlayer->PrepareGossipMenu(pCreature, GOSSIP_ID_MULVERICK1);

		if (npc_wing_commander_mulverickAI* pEscortAI = dynamic_cast<npc_wing_commander_mulverickAI*>(pCreature->AI()))
			pPlayer->ADD_GOSSIP_ITEM_ID(GOSSIP_ICON_CHAT, GOSSIP_ITEM_MULVERICK_ESCORT, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 1);

		pPlayer->SendPreparedGossip(pCreature);
	}
	else if (pCreature->GetMotionMaster()->getLastReachedWaypoint() > 10 && pCreature->HasFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_GOSSIP))
	{
		pPlayer->PrepareGossipMenu(pCreature, GOSSIP_ID_MULVERICK2);

		if (pPlayer->GetReputationRank(729) > 5)
		{
			if (!pPlayer->HasItemCount(ITEM_MULVERICK_BEACON, 1, true))
			{
				pPlayer->ADD_GOSSIP_ITEM_ID(GOSSIP_ICON_CHAT, GOSSIP_ITEM_MULVERICK_BEACON, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 2);
			}
			pPlayer->ADD_GOSSIP_ITEM_ID(GOSSIP_ICON_CHAT, GOSSIP_ITEM_MULVERICK_FLIGHT, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 3);
		}
		pPlayer->SendPreparedGossip(pCreature);
	}
	else if (pCreature->GetMotionMaster()->getLastReachedWaypoint() > 1 && pCreature->HasFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_QUESTGIVER))
	{
		pPlayer->PrepareQuestMenu(pCreature->GetGUID());
		pPlayer->SendPreparedQuest(pCreature->GetGUID());
	}
	return true;
}

bool GossipSelect_npc_wing_commander_mulverick(Player* pPlayer, Creature* pCreature, uint32 /*uiSender*/, uint32 uiAction)
{
	npc_wing_commander_mulverickAI* pEscortAI = dynamic_cast<npc_wing_commander_mulverickAI*>(pCreature->AI());
	switch (uiAction)
	{
	case GOSSIP_ACTION_INFO_DEF + 1:
		pPlayer->CLOSE_GOSSIP_MENU();
		pEscortAI->StartEscort();
		break;
	case GOSSIP_ACTION_INFO_DEF + 2:
		pPlayer->CLOSE_GOSSIP_MENU();
		pCreature->CastSpell(pPlayer, SPELL_CONJURE_MULVERICK_BEACON, TRIGGERED_NONE);
		break;
	case GOSSIP_ACTION_INFO_DEF + 3:
		pPlayer->CLOSE_GOSSIP_MENU();
		pEscortAI->StartPatrol();
		break;
	default:
		return false;
	}
	return true;                                            // no default handling  -> prevent mangos core handling	
}


enum
{
	NPC_AERIE_GRYPHON			= 13161,
	NPC_WAR_RIDER				= 13178,

	SPELL_STUN_BOMB_ATTACK		= 21188,
	SPELL_FIREBALL				= 22088,
	SPELL_FIREBALL_VOLLEY		= 15285,
	SPELL_THROW_LIQUID_FIRE		= 23969
};

struct aerial_assault_creatureAI : public ScriptedAI
{
	aerial_assault_creatureAI(Creature* pCreature) : ScriptedAI(pCreature) { Reset(); }

	uint32 m_uiAttackTimer;

	void Reset() override
	{
		m_uiAttackTimer = 10000;
	}

	void Aggro(Unit* pWho) override
	{
		m_uiAttackTimer = 1000;
	}

	bool CanAttack(Player* pPlayer)
	{
		if (m_creature->getFaction() == 1217 && pPlayer->GetTeam() == ALLIANCE)
			return false;
		else if (m_creature->getFaction() == 1214 && pPlayer->GetTeam() == HORDE)
			return false;
		else
			return true;
	}

	void UpdateAI(const uint32 uiDiff) override
	{
		if (!m_creature->getVictim())
		{
			Player* pTarget;
			std::list<Player*> lPlayers;
			GetPlayerListWithEntryInWorld(lPlayers, m_creature, 80.0f);
			if (!lPlayers.empty())
			{
				std::list<Player*>::iterator i = lPlayers.begin();
				advance(i, (rand() % lPlayers.size()));
				pTarget = (*i);

				if (CanAttack(pTarget) == true)
				{
					if (pTarget)
					{
						m_creature->GetMotionMaster()->MoveIdle();
						m_creature->Attack(pTarget, false);
						m_creature->AddThreat(pTarget);
						m_creature->SetInCombatWith(pTarget);
						pTarget->SetInCombatWith(m_creature);
					}
				}
			}
		}

		Unit* m_victim = m_creature->getVictim();
		if (m_victim)
		{
			switch (m_creature->GetEntry())
			{
			case NPC_AERIE_GRYPHON:
			case NPC_WAR_RIDER:
				if (m_uiAttackTimer <= uiDiff)
				{
					if (urand(1, 2) == 1)
						m_creature->CastSpell(m_victim, SPELL_STUN_BOMB_ATTACK, TRIGGERED_OLD_TRIGGERED);
					else
						m_creature->CastSpell(m_victim, SPELL_FIREBALL, TRIGGERED_OLD_TRIGGERED);

					m_uiAttackTimer = 10000;
				}
				else
					m_uiAttackTimer -= uiDiff;
				break;
			case NPC_SLIDORE_GRYPHON:
			case NPC_VIPORE_GRYPHON:
			case NPC_ICHMAN_GRYPHON:
			case NPC_GUSE_WAR_RIDER:
			case NPC_JEZTOR_WAR_RIDER:
			case NPC_MULVERICK_WAR_RIDER:
				if (m_uiAttackTimer <= uiDiff)
				{
					switch (urand(1, 3))
					{
					case 1:
						m_creature->CastSpell(m_victim, SPELL_STUN_BOMB_ATTACK, TRIGGERED_OLD_TRIGGERED);
						m_uiAttackTimer = 10000;
						break;
					case 2:
						m_creature->CastSpell(m_victim, SPELL_FIREBALL_VOLLEY, TRIGGERED_OLD_TRIGGERED);
						m_uiAttackTimer = 10000;
						break;
					case 3 :
						m_creature->CastSpell(m_victim, SPELL_THROW_LIQUID_FIRE, TRIGGERED_OLD_TRIGGERED);
						m_uiAttackTimer = 10000;
						break;
					}
				}
				else
					m_uiAttackTimer -= uiDiff;
				break;
			}

			if (m_victim->GetDistance(m_creature) >= 80.0f)
			{
				m_creature->AI()->EnterEvadeMode();
			}
			if (m_victim->isDead())
			{
				m_creature->AI()->EnterEvadeMode();
			}
		}
	}
};

CreatureAI* GetAI_aerial_assault_creature(Creature* pCreature)
{
	return new aerial_assault_creatureAI(pCreature);
}


void AddSC_battleground()
{
	Script* pNewScript;

	pNewScript = new Script;
	pNewScript->Name = "npc_spirit_guide";
	pNewScript->GetAI = &GetAI_npc_spirit_guide;
	pNewScript->pGossipHello = &GossipHello_npc_spirit_guide;
	pNewScript->RegisterSelf();

	pNewScript = new Script;
	pNewScript->Name = "npc_wing_commander_slidore";
	pNewScript->GetAI = &GetAI_npc_wing_commander_slidore;
	pNewScript->pGossipHello = &GossipHello_npc_wing_commander_slidore;
	pNewScript->pGossipSelect = &GossipSelect_npc_wing_commander_slidore;
	pNewScript->RegisterSelf();

	pNewScript = new Script;
	pNewScript->Name = "npc_wing_commander_vipore";
	pNewScript->GetAI = &GetAI_npc_wing_commander_vipore;
	pNewScript->pGossipHello = &GossipHello_npc_wing_commander_vipore;
	pNewScript->pGossipSelect = &GossipSelect_npc_wing_commander_vipore;
	pNewScript->RegisterSelf();

	pNewScript = new Script;
	pNewScript->Name = "npc_wing_commander_ichman";
	pNewScript->GetAI = &GetAI_npc_wing_commander_ichman;
	pNewScript->pGossipHello = &GossipHello_npc_wing_commander_ichman;
	pNewScript->pGossipSelect = &GossipSelect_npc_wing_commander_ichman;
	pNewScript->RegisterSelf();

	pNewScript = new Script;
	pNewScript->Name = "npc_wing_commander_guse";
	pNewScript->GetAI = &GetAI_npc_wing_commander_guse;
	pNewScript->pGossipHello = &GossipHello_npc_wing_commander_guse;
	pNewScript->pGossipSelect = &GossipSelect_npc_wing_commander_guse;
	pNewScript->RegisterSelf();

	pNewScript = new Script;
	pNewScript->Name = "npc_wing_commander_jeztor";
	pNewScript->GetAI = &GetAI_npc_wing_commander_jeztor;
	pNewScript->pGossipHello = &GossipHello_npc_wing_commander_jeztor;
	pNewScript->pGossipSelect = &GossipSelect_npc_wing_commander_jeztor;
	pNewScript->RegisterSelf();

	pNewScript = new Script;
	pNewScript->Name = "npc_wing_commander_mulverick";
	pNewScript->GetAI = &GetAI_npc_wing_commander_mulverick;
	pNewScript->pGossipHello = &GossipHello_npc_wing_commander_mulverick;
	pNewScript->pGossipSelect = &GossipSelect_npc_wing_commander_mulverick;
	pNewScript->RegisterSelf();

	pNewScript = new Script;
	pNewScript->Name = "npc_aerial_assault";
	pNewScript->GetAI = &GetAI_aerial_assault_creature;
	pNewScript->RegisterSelf();
}
