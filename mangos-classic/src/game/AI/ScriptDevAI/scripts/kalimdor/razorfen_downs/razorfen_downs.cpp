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
SDName: Razorfen_Downs
SD%Complete: 100
SDComment: Quest 3525
SDCategory: Razorfen Downs
EndScriptData

*/

#include "AI/ScriptDevAI/PreCompiledHeader.h"/* ContentData
npc_belnistrasz
EndContentData */

#include "razorfen_downs.h"
#include "AI/ScriptDevAI/base/escort_ai.h"

/*###
# npc_belnistrasz
####*/

enum
{
    QUEST_EXTINGUISHING_THE_IDOL    = 3525,

    SAY_BELNISTRASZ_READY           = -1129005,
    SAY_BELNISTRASZ_START_RIT       = -1129006,
    SAY_BELNISTRASZ_AGGRO_1         = -1129007,
    SAY_BELNISTRASZ_AGGRO_2         = -1129008,
    SAY_BELNISTRASZ_3_MIN           = -1129009,
    SAY_BELNISTRASZ_2_MIN           = -1129010,
    SAY_BELNISTRASZ_1_MIN           = -1129011,
    SAY_BELNISTRASZ_FINISH          = -1129012,

    NPC_IDOL_ROOM_SPAWNER           = 8611,

    NPC_WITHERED_BATTLE_BOAR        = 7333,
    NPC_WITHERED_QUILGUARD          = 7329,
    NPC_DEATHS_HEAD_GEOMANCER       = 7335,
	//NPC_PLAGUEMAW_THE_ROTTING = 7356, // Correctif, déplacé dans razorfen_downs.h
    
	GO_BELNISTRASZ_BRAZIER          = 152097,

    SPELL_ARCANE_INTELLECT          = 13326,                // use this somewhere (he has it as default)
    SPELL_FIREBALL                  = 9053,
    SPELL_FROST_NOVA                = 11831,
    SPELL_IDOL_SHUTDOWN             = 12774,

    // summon spells only exist in 1.x
    // SPELL_SUMMON_1                  = 12694,             // NPC_WITHERED_BATTLE_BOAR
    // SPELL_SUMMON_2                  = 14802,             // NPC_DEATHS_HEAD_GEOMANCER
    // SPELL_SUMMON_3                  = 14801,             // NPC_WITHERED_QUILGUARD
};

static float m_fSpawnerCoord[3][4] =
{
    {2582.79f, 954.392f, 52.4821f, 3.78736f},
    {2569.42f, 956.380f, 52.2732f, 5.42797f},
    {2570.62f, 942.393f, 53.7433f, 0.71558f}
};

struct npc_belnistraszAI : public npc_escortAI
{
    npc_belnistraszAI(Creature* pCreature) : npc_escortAI(pCreature)
    {
		m_pInstance = (instance_razorfen_downs*)pCreature->GetInstanceData();
		m_uiRitualPhase = 0;
        m_uiRitualTimer = 1000;
        m_bAggro = false;
        Reset();
    }

	instance_razorfen_downs* m_pInstance;
    uint8 m_uiRitualPhase;
    uint32 m_uiRitualTimer;
    bool m_bAggro;

    uint32 m_uiFireballTimer;
    uint32 m_uiFrostNovaTimer;

    void Reset() override
    {
        m_uiFireballTimer  = 1000;
        m_uiFrostNovaTimer = 6000;
    }

    void AttackedBy(Unit* pAttacker) override
    {
        if (HasEscortState(STATE_ESCORT_PAUSED))
        {
            if (!m_bAggro)
            {
                DoScriptText(urand(0, 1) ? SAY_BELNISTRASZ_AGGRO_1 : SAY_BELNISTRASZ_AGGRO_2, m_creature, pAttacker);
                m_bAggro = true;
            }

            return;
        }

        ScriptedAI::AttackedBy(pAttacker);
    }

	void SpawnerSummon(Creature* pSummoner)
	{
		if (m_uiRitualPhase > 9) // Correctif, ajout de phases manquantes
		{
			pSummoner->SummonCreature(NPC_PLAGUEMAW_THE_ROTTING, pSummoner->GetPositionX(), pSummoner->GetPositionY(), pSummoner->GetPositionZ(), pSummoner->GetOrientation(), TEMPSUMMON_TIMED_OOC_DESPAWN, 60000);
			return;
		}

		int i = rand()%2; // Correctif, nombre de sanglier aléatoire
		
		for (i; i < 4; ++i)
		{
			uint32 uiEntry = 0;

			// ref TARGET_RANDOM_CIRCUMFERENCE_POINT
			float angle = 2.0f * M_PI_F * rand_norm_f();
			float fX, fZ, fY;
			pSummoner->GetClosePoint(fX, fZ, fY, 0.0f, 2.0f, angle);

			switch (i)
			{
			case 0: // Correctif, 2 sangliers
				uiEntry = NPC_WITHERED_BATTLE_BOAR;
				break;
			case 1: // 1 sanglier
				uiEntry = NPC_WITHERED_BATTLE_BOAR;
				break;
			case 2:
				uiEntry = NPC_WITHERED_QUILGUARD;
				break;
			case 3:
				uiEntry = NPC_DEATHS_HEAD_GEOMANCER;
				break;
			}

			pSummoner->SummonCreature(uiEntry, fX, fZ, fY, 0.0f, TEMPSUMMON_TIMED_OOC_DESPAWN, 60000);
		}
	}

    void JustSummoned(Creature* pSummoned) override
    {
        SpawnerSummon(pSummoned);
    }

    void DoSummonSpawner(int32 iType)
    {
        m_creature->SummonCreature(NPC_IDOL_ROOM_SPAWNER, m_fSpawnerCoord[iType][0], m_fSpawnerCoord[iType][1], m_fSpawnerCoord[iType][2], m_fSpawnerCoord[iType][3], TEMPSUMMON_TIMED_DESPAWN, 10000);
    }

    void WaypointReached(uint32 uiPointId) override
    {
        if (uiPointId == 24)
        {
            DoScriptText(SAY_BELNISTRASZ_START_RIT, m_creature);
            SetEscortPaused(true);
			m_pInstance->SetData(TYPE_PLAGUEMAW_THE_ROTTING, IN_PROGRESS); // Correctif, l'event ne doit pas se finir tant que le boss n'est pas down
        }
    }

    void UpdateEscortAI(const uint32 uiDiff) override
    {
        if (HasEscortState(STATE_ESCORT_PAUSED))
        {
			if (m_uiRitualTimer < uiDiff)
            {
				switch (m_uiRitualPhase)
                {
                    // Correctif, ajout d'une vague manquant, correction des timers
					case 0:
                        DoCastSpellIfCan(m_creature, SPELL_IDOL_SHUTDOWN);
						DoSummonSpawner(irand(1, 3)); // 1ere vague
                        m_uiRitualTimer = 35000;
                        break;
                    case 1:
                        DoSummonSpawner(irand(1, 3)); // 2eme vague
                        m_uiRitualTimer = 4000;
                        break;
					case 2:
						DoScriptText(SAY_BELNISTRASZ_3_MIN, m_creature, m_creature);
						m_uiRitualTimer = 31000;
						break;
                    case 3:
                        DoSummonSpawner(irand(1, 3)); // 3eme vague
                        m_uiRitualTimer = 35000;
                        break;
                    case 4:
                        DoSummonSpawner(irand(1, 3)); // 4eme vague
                        m_uiRitualTimer = 13000;
                        break;
                    case 5:
                        DoScriptText(SAY_BELNISTRASZ_2_MIN, m_creature, m_creature);
                        m_uiRitualTimer = 20000;
                        break;
                    case 6:
                        DoSummonSpawner(irand(1, 3)); //5eme vague
                        m_uiRitualTimer = 35000;
                        break;
					case 7:
						DoSummonSpawner(irand(1, 3)); //6eme vague
						m_uiRitualTimer = 20000;
						break; 
					case 8:
                        DoScriptText(SAY_BELNISTRASZ_1_MIN, m_creature, m_creature);
                        m_uiRitualTimer = 40000;
                        break;
                    case 9:
                        DoSummonSpawner(irand(1, 3)); //7eme vague
                        m_uiRitualTimer = 12000;
                        break;
					case 10:
						DoSummonSpawner(irand(1, 3)); //Boss
						m_uiRitualTimer = 3000;
						break;
                    case 11:
                        DoScriptText(SAY_BELNISTRASZ_FINISH, m_creature, m_creature);
                        m_uiRitualTimer = 3000;
                        break;
                    case 12:
                    {
                        if (Player* pPlayer = GetPlayerForEscort())
                        {
                            pPlayer->GroupEventHappens(QUEST_EXTINGUISHING_THE_IDOL, m_creature);

                            if (GameObject* pGo = GetClosestGameObjectWithEntry(m_creature, GO_BELNISTRASZ_BRAZIER, 10.0f))
                            {
                                if (!pGo->isSpawned())
                                {
                                    pGo->SetRespawnTime(HOUR * IN_MILLISECONDS);
                                    pGo->Refresh();
                                }
                            }
                        }

                        m_creature->RemoveAurasDueToSpell(SPELL_IDOL_SHUTDOWN);
                        SetEscortPaused(false);
                        break;
                    }
                }
                ++m_uiRitualPhase;
            }
			else if((!(m_uiRitualPhase == 11)) || (m_pInstance && m_pInstance->GetData(TYPE_PLAGUEMAW_THE_ROTTING) == DONE)) //Correctif, empêche l'event de s'achever tant que le boss n'est pas down
			{
				m_uiRitualTimer -= uiDiff;
			}

            return;
        }

        if (!m_creature->SelectHostileTarget() || !m_creature->getVictim())
            return;

        if (m_uiFireballTimer < uiDiff)
        {
            DoCastSpellIfCan(m_creature->getVictim(), SPELL_FIREBALL);
            m_uiFireballTimer  = urand(2000, 3000);
        }
        else
            m_uiFireballTimer -= uiDiff;

        if (m_uiFrostNovaTimer < uiDiff)
        {
            DoCastSpellIfCan(m_creature->getVictim(), SPELL_FROST_NOVA);
            m_uiFrostNovaTimer = urand(10000, 15000);
        }
        else
            m_uiFrostNovaTimer -= uiDiff;

        DoMeleeAttackIfReady();
    }
};

CreatureAI* GetAI_npc_belnistrasz(Creature* pCreature)
{
    return new npc_belnistraszAI(pCreature);
}

bool QuestAccept_npc_belnistrasz(Player* pPlayer, Creature* pCreature, const Quest* pQuest)
{
    if (pQuest->GetQuestId() == QUEST_EXTINGUISHING_THE_IDOL)
    {
        if (npc_belnistraszAI* pEscortAI = dynamic_cast<npc_belnistraszAI*>(pCreature->AI()))
        {
            pEscortAI->Start(true, pPlayer, pQuest);
            DoScriptText(SAY_BELNISTRASZ_READY, pCreature, pPlayer);
            pCreature->SetFactionTemporary(FACTION_ESCORT_N_NEUTRAL_ACTIVE, TEMPFACTION_RESTORE_RESPAWN);
        }
    }

    return true;
}

void AddSC_razorfen_downs()
{
    Script* pNewScript;

    pNewScript = new Script;
    pNewScript->Name = "npc_belnistrasz";
    pNewScript->GetAI = &GetAI_npc_belnistrasz;
    pNewScript->pQuestAcceptNPC = &QuestAccept_npc_belnistrasz;
    pNewScript->RegisterSelf();
}
