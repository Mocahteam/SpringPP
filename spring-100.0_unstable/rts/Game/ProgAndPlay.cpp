/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

// Muratet (Implement class CProgAndPlay) ---

//#include "StdAfx.h" //Bontemps (changed from 0.82.5.1)
//#include "mmgr.h" //Bontemps (changed from 0.82.5.1)

#include "ProgAndPlay.h"
#include "lib/pp/PP_Supplier.h"
#include "lib/pp/PP_Error.h"
#include "lib/pp/PP_Error_Private.h"

//#include "System/NetProtocol.h" //Bontemps (changed from 0.82.5.1)
#include "Net/Protocol/NetProtocol.h" //Bontemps (changed from 0.82.5.1)
#include "Game/Game.h"
#include "Sim/Misc/GlobalConstants.h" // needed for MAX_UNITS
#include "Sim/Misc/TeamHandler.h"
#include "Sim/Misc/LosHandler.h"
#include "Sim/Features/FeatureHandler.h"
#include "Sim/Features/FeatureSet.h"
//#include "Sim/Units/Groups/GroupHandler.h" //Bontemps (changed from 0.82.5.1)
#include "Game/UI/Groups/GroupHandler.h"
//#include "Sim/Units/Groups/Group.h" //Bontemps (changed from 0.82.5.1)
#include "Game/UI/Groups/Group.h"
#include "Sim/Units/CommandAI/Command.h"
#include "Sim/Units/CommandAI/CommandAI.h"
#include "Sim/Units/CommandAI/FactoryCAI.h"
//#include "FileSystem/FileHandler.h" //Bontemps (changed from 0.82.5.1)
//#include "FileSystem/FileSystem.h" //Bontemps (changed from 0.82.5.1)
//#include "FileSystem/FileSystemHandler.h" //Bontemps (changed from 0.82.5.1)
#include "System/FileSystem/FileHandler.h"
#include "System/FileSystem/FileSystem.h"
#include "System/FileSystem/DataDirsAccess.h" //Bontemps (changed from 0.82.5.1)
#include "GlobalUnsynced.h"
//#include "LogOutput.h" //Bontemps (changed from 0.82.5.1)
#include "System/LogOutput.h"
#include "Map/ReadMap.h"
#include "Sim/Misc/SmoothHeightMesh.h" //Bontemps (changed from 0.82.5.1)
#include "ExternalAI/SkirmishAIHandler.h" //Bontemps (changed from 0.82.5.1)
#include "Sim/Misc/TeamHandler.h" //Bontemps (changed from 0.82.5.1)

#include <fstream>
#include <sstream>
std::ofstream logFile("log.txt");

void log(std::string msg){
	//logFile << msg << std::endl;
	//logOutput.Print(msg.c_str());
}

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CProgAndPlay* pp = NULL;

CProgAndPlay::CProgAndPlay()
{
log("ProgAndPLay constructor begin");
	loaded = false;
	updated = false;
	// initialisation of Prog&Play
	if (PP_Init() == -1){
		std::string tmp(PP_GetError());
		log(tmp.c_str());
	}
	else{
		log("Prog&Play initialized");
		loaded = true;
	}
	
	// delete mission_ended.conf file if it exists => this file could be created
	// by mods if lua mission is ended.
	CFileHandler * tmpFile = new CFileHandler("mission_ended.conf");
	bool del = tmpFile->FileExists();
	// free CFileHandler before deleting the file, otherwise it is blocking on Windows
	delete tmpFile;
	if (del)
		//FileSystemHandler::DeleteFile(filesystem.LocateFile("mission_ended.conf")); //Bontemps (changed from 0.82.5.1)
		FileSystem::DeleteFile(dataDirsAccess.LocateFile("mission_ended.conf")); //Bontemps (changed from 0.82.5.1)
log("ProgAndPLay constructor end");
}

CProgAndPlay::~CProgAndPlay()
{
log("ProgAndPLay destructor begin");
  if (loaded){
    if (PP_Quit() == -1){
		std::string tmp(PP_GetError());
		log(tmp.c_str());
	}
	else
		log("Prog&Play shut down and cleaned up");
  }
log("ProgAndPLay destructor end");
}

void CProgAndPlay::Update(void)
{
log("ProgAndPLay::Update begin");
	// Execute pending commands
	int nbCmd = execPendingCommands();
	if (nbCmd == -1){
		std::string tmp(PP_GetError());
		log(tmp.c_str());
	}
	// Limit update if commands was executed or every 4 frames
	if (updatePP() == -1){
		std::string tmp(PP_GetError());
		log(tmp.c_str());
	}
		
log("ProgAndPLay::Update end");
}

PP_ShortUnit buildShortUnit(CUnit *unit, PP_Coalition c){
log("ProgAndPLay::buildShortUnit begin");
	PP_ShortUnit tmpUnit;
	tmpUnit.id = unit->id;
	tmpUnit.coalition = c;
	if (unit->unitDef)
		tmpUnit.type = unit->unitDef->id;
	else
		tmpUnit.type = -1;
	tmpUnit.pos.x = unit->midPos.x;
	tmpUnit.pos.y = unit->midPos.z;
	tmpUnit.health = unit->health;
	tmpUnit.maxHealth = unit->maxHealth;
	// write command queue and group
	if (c != ENEMY_COALITION){
		if (unit->group)
			tmpUnit.group = unit->group->id;
		else
			tmpUnit.group = -1;
		std::vector<PP_Command> tmpCommandQueue;
		const CCommandAI* commandAI = unit->commandAI;
		if (commandAI != NULL) {
			const CCommandQueue* queue;
			queue = &commandAI->commandQue;
			CCommandQueue::const_iterator it;
			for (it = queue->begin(); it != queue->end(); it++) {
				if (it->GetID() != CMD_SET_WANTED_MAX_SPEED){ //Bontemps (changed from 0.82.5.1)
					PP_Command tmpCmd;
					tmpCmd.code = it->GetID(); //Bontemps (changed from 0.82.5.1)
					tmpCmd.nbParam = it->params.size();
					tmpCmd.param = (float*)malloc(tmpCmd.nbParam*sizeof(float));
					if (tmpCmd.param != NULL){
						for (int i = 0 ; i < tmpCmd.nbParam ; i++){
							tmpCmd.param[i] = it->params.at(i);
						}
					}
					else
						tmpCmd.nbParam = 0;
					tmpCommandQueue.push_back(tmpCmd);
				}
			}
		}
		tmpUnit.commandQueue =
			(PP_Command*)malloc(tmpCommandQueue.size()*sizeof(PP_Command));
		if (tmpUnit.commandQueue != NULL){
			tmpUnit.nbCommandQueue = tmpCommandQueue.size();
			for (int i = 0 ; i < tmpUnit.nbCommandQueue ; i++)
				tmpUnit.commandQueue[i] = tmpCommandQueue.at(i);
		}
		else
			tmpUnit.nbCommandQueue = 0;
	}
	else{
		tmpUnit.group = -1;
		tmpUnit.commandQueue = NULL;
		tmpUnit.nbCommandQueue = 0;
	}
	
log("ProgAndPLay::buildShortUnit end");
	return tmpUnit;
}

void freeShortUnit (PP_ShortUnit unit){
log("ProgAndPLay::freeShortUnit begin");
	if (unit.commandQueue != NULL){
		for (int i = 0 ; i < unit.nbCommandQueue ; i++){
			if (unit.commandQueue[i].param != NULL)
				free(unit.commandQueue[i].param);
		}
		free (unit.commandQueue);
		unit.commandQueue = NULL;
		unit.nbCommandQueue = 0;
	}
log("ProgAndPLay::freeShortUnit end");
}

void doUpdate(CUnit* unit, PP_Coalition c){
std::stringstream str;
str << "ProgAndPLay::doUpdate begin : " << unit->id;
log(str.str());
	PP_ShortUnit shortUnit = buildShortUnit(unit, c);
	if (PP_UpdateUnit(shortUnit) == -1){
		std::string tmp(PP_GetError());
		log(tmp.c_str());
	}
	freeShortUnit(shortUnit);
log("ProgAndPLay::doUpdate end");
}

void doAdding(CUnit* unit, PP_Coalition c){
std::stringstream str;
str << "ProgAndPLay::doAdding begin : " << unit->id;
log(str.str());
	PP_ShortUnit shortUnit = buildShortUnit(unit, c);
	if (PP_AddUnit(shortUnit) == -1){
		std::string tmp(PP_GetError());
		log(tmp.c_str());
	}
	freeShortUnit(shortUnit);
log("ProgAndPLay::doAdding end");
}

void doRemoving (int unitId){
std::stringstream ss;
ss << "ProgAndPLay::doRemoving begin : " << unitId;
log(ss.str());
	if (PP_RemoveUnit(unitId) == -1){
		std::string tmp(PP_GetError());
		log(tmp.c_str());
	}
log("ProgAndPLay::doRemoving end");
}

void CProgAndPlay::AddUnit(CUnit* unit){
log("ProgAndPLay::AddUnit begin");
	PP_Coalition c;
	if (unit->team == gu->myTeam)
		c = MY_COALITION;
	else if (teamHandler->AlliedTeams(unit->team, gu->myTeam))
		c = ALLY_COALITION;
	else 
		c = ENEMY_COALITION;
	if (c != ENEMY_COALITION || losHandler->InLos(unit, gu->myAllyTeam)){ //Bontemps (changed from 0.82.5.1)
		doAdding(unit, c);
	}
log("ProgAndPLay::AddUnit end");
}

void CProgAndPlay::UpdateUnit(CUnit* unit){
log("ProgAndPLay::UpdateUnit begin");
	PP_Coalition c;
	if (unit->team == gu->myTeam)
		c = MY_COALITION;
	else if (teamHandler->AlliedTeams(unit->team, gu->myTeam))
		c = ALLY_COALITION;
	else 
		c = ENEMY_COALITION;
	if (c != ENEMY_COALITION){
		doUpdate(unit, c);
	}
	else {
		int isStored = PP_IsStored(unit->id);
		if (isStored == -1){
			std::string tmp(PP_GetError());
			log(tmp.c_str());
		}
		else {
			if (losHandler->InLos(unit, gu->myAllyTeam)){ //Bontemps (changed from 0.82.5.1)
				if (isStored){
					doUpdate(unit, c);
				}
				else{
					doAdding(unit, c);
				}
			}
			else {
				if (isStored){
					doRemoving(unit->id);
				}
			}
		}
	}
log("ProgAndPLay::UpdateUnit end");
}

void CProgAndPlay::RemoveUnit(CUnit* unit){
log("ProgAndPLay::RemoveUnit begin");
	doRemoving(unit->id);
log("ProgAndPLay::RemoveUnit end");
}

int CProgAndPlay::updatePP(void){
log("ProgAndPLay::updatePP begin");
	if (!updated) {
		updated = true;
		// store map size
		PP_Pos mapSize;
		mapSize.x = mapDims.mapx*SQUARE_SIZE; //Bontemps (changed from 0.82.5.1)
		mapSize.y = mapDims.mapy*SQUARE_SIZE; //Bontemps (changed from 0.82.5.1)
		// store strating position
		PP_Pos startPos;
		startPos.x = teamHandler->Team(gu->myTeam)->GetStartPos().x;
		startPos.y = teamHandler->Team(gu->myTeam)->GetStartPos().z;
		// store all geothermals
		PP_Positions specialAreas;
		specialAreas.size = 0;
		specialAreas.pos = NULL;
		if (featureHandler){
			CFeatureSet fset = featureHandler->GetActiveFeatures();
			if (fset.size() > 0){
				specialAreas.pos = (PP_Pos*) malloc(fset.size()*sizeof(PP_Pos));
				if (specialAreas.pos == NULL){
					PP_SetError("Spring : specialAres allocation error");
					return -1;
				}
				CFeatureSet::const_iterator fIt;
				for (fIt = fset.begin() ; fIt != fset.end(); ++fIt){
					if (!(*fIt)->def) continue;
					if ((*fIt)->def->geoThermal){
						specialAreas.pos[specialAreas.size].x = (*fIt)->pos.x;
						specialAreas.pos[specialAreas.size].y = (*fIt)->pos.z;
						specialAreas.size++;
					}
				}
				PP_Pos *tmp = (PP_Pos*)
					realloc(specialAreas.pos, specialAreas.size*sizeof(PP_Pos));
				if (tmp == NULL){
					PP_SetError("Spring : specialAres reallocation error");
					free(specialAreas.pos);
					specialAreas.size = 0;
					specialAreas.pos = NULL;
					return -1;
				}
				else
					specialAreas.pos = tmp;
			}
		}
		int ret = PP_SetStaticData(mapSize, startPos, specialAreas);
		free(specialAreas.pos);
		if (ret == -1)
			return -1;
	}
	
	// store resources
	PP_Resources resources;
	resources.size = 2;
	resources.resource = (int*) malloc(sizeof(int)*2);
	if (resources.resource == NULL){
		PP_SetError("Spring : Ressources allocation error");
		return -1;
	}
	resources.resource[0] = teamHandler->Team(gu->myTeam)->res.metal; //Bontemps (changed from 0.82.5.1)
	resources.resource[1] = teamHandler->Team(gu->myTeam)->res.energy; //Bontemps (changed from 0.82.5.1)
	int ret = PP_SetRessources(resources);
	free(resources.resource);
	if (ret == -1)
		return -1;
	
	// set game over. This depends on engine state (game->gameOver) and/or
	// missions state (tmpFile->FileExists() => this file is created by mod if 
	// lua mission is ended)
	CFileHandler * tmpFile = new CFileHandler("mission_ended.conf");
	ret = PP_SetGameOver(game->IsGameOver() || tmpFile->FileExists()); //Bontemps (changed from 0.82.5.1)
	delete tmpFile;
	if (ret == -1)
		return -1;

log("ProgAndPLay::updatePP end");
	return 0;
}

int CProgAndPlay::execPendingCommands(){
log("ProgAndPLay::execPendingCommands begin");
	int nbCmd = 0;
	PP_PendingCommands* pendingCommands = PP_GetPendingCommands();
	if (pendingCommands == NULL) return -1;
	else{
		nbCmd = pendingCommands->size;
		for (int i = 0 ; i < pendingCommands->size ; i++){
			PP_PendingCommand pc = pendingCommands->pendingCommand[i];
			// Check for affecting group
			if (pc.group != -2){
				// Check if unit exists
				CUnit* tmp = unitHandler->GetUnit(pc.unitId);
				if (tmp != NULL){
					if (tmp->team == gu->myTeam){
						// Check the grouphandlers for my team
						if (grouphandlers[gu->myTeam]){
							// Check if it's a command to withdraw unit from its current group
							if (pc.group == -1){
								// suppression de l'unité de sons groupe
								tmp->SetGroup(NULL);
							}
							else{
								// Check if the number of groups is being enough
								while (pc.group >= grouphandlers[gu->myTeam]->groups.size()){
									// add a new group
									grouphandlers[gu->myTeam]->CreateNewGroup();
								}
								// Check if the group exists
								if (!grouphandlers[gu->myTeam]->groups[pc.group]){
									// recreate all groups until recreate this group
									CGroup * gTmp;
									do {
										// création d'un groupe intermédiaire manquant
										gTmp = grouphandlers[gu->myTeam]->CreateNewGroup();
									}
									while (gTmp->id != pc.group);
								}
								if (pc.group >= 0){
									// devote the unit to this group
									tmp->SetGroup(grouphandlers[gu->myTeam]->groups[pc.group]);
								}
							}
						}
					}
				}
			}
			// Check if a command must be executed
			bool ok = true, found = false;
			switch (pc.commandType){
				case -1 : // the command is invalid => do not execute this command
					ok = false;
					break;
				case 0 : // the command target a position
					// indicates the y value
					pc.command.param[1] = 
						smoothGround->GetHeight(pc.command.param[0], pc.command.param[2]);
					break;
				case 1 : // the command targets a unit
					// Find targeted unit
					for (int t = 0 ; t < teamHandler->ActiveTeams() && !found ; t++){
						CUnit* tmp = unitHandler->GetUnit((int)pc.command.param[0]);
						if (tmp != NULL){
							if (tmp->team == t){
								// check if this unit is visible by the player
								if (!losHandler->InLos(tmp, gu->myAllyTeam)) //Bontemps (changed from 0.82.5.1)
									ok = false;
								found = true;
							}
						}
					}
					break;
				case 2 : // the command is untargeted
					// nothing to do
					break;
				default :
					PP_SetError("Spring : commandType unknown");
					PP_FreePendingCommands(pendingCommands);
					return -1;
			}
			// send command
			if (ok){
				if (pc.command.code == -7658){ // This code is a Prog&Play reserved code defined in constantList files
					// Clear CommandQueue of factory
					CUnit* fact = unitHandler->GetUnit(pc.unitId);
					if (fact){
						CFactoryCAI* facAI = dynamic_cast<CFactoryCAI*>(fact->commandAI);
						if (facAI) {
							CCommandQueue& buildCommands = facAI->commandQue;
							CCommandQueue::iterator it;
							std::vector<Command> clearCommands;
							clearCommands.reserve(buildCommands.size());
							for (it = buildCommands.begin(); it != buildCommands.end(); ++it) {
								Command c(it->GetID());
								c.options = RIGHT_MOUSE_KEY; // Set RIGHT_MOUSE_KEY option in order to decrement building order
								clearCommands.push_back(c);
							}
							for (int i = 0; i < (int)clearCommands.size(); i++) {
								facAI->GiveCommand(clearCommands[i]);
							}
						}
					}
				} else {
					// Copy pc.command.param in a vector to pass it at SendAICommand
					safe_vector<float> stdParam;
					for (int j = 0 ; j < pc.command.nbParam ; j++) {
						stdParam.push_back(pc.command.param[j]);
					}
					/*
					clientNet->Send(CBaseNetProtocol::Get().SendAICommand(gu->myPlayerNum, skirmishAIHandler.GetCurrentAIID(), pc.unitId, //Bontemps (changed from 0.82.5.1)
						pc.command.code, pc.command.code, 0, stdParam));
					*/
					// Permet de gérer les commandes plus correctement mais produit des
					// erreurs de synchronisation (à utiliser en SYNCED_MODE)
					CUnitSet *tmp = &(teamHandler->Team(gu->myTeam)->units); //Bontemps (changed from 0.82.5.1)
					for (CUnitSet::const_iterator it = tmp->begin(); it != tmp->end(); ++it) {
						if ((*it)->id == pc.unitId) {
							if ((*it)->commandAI){
								Command cTmp(pc.command.code);
								cTmp.params = stdParam;
								(*it)->commandAI->GiveCommand(cTmp);
							}
							break;
						}						
					}
				}
			}
		}
	}
	PP_FreePendingCommands(pendingCommands);
log("ProgAndPLay::execPendingCommands end");
	return nbCmd;
}

// ---
