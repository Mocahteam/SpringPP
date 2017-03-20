/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

// Muratet (Implement class CProgAndPlay) ---

#include <fstream>
#include <sstream>
#include <iostream>
#include <map>
#include <cmath>

#include "StdAfx.h"
#include "mmgr.h"

#include "ProgAndPlay.h"
#include "lib/pp/PP_Supplier.h"
#include "lib/pp/PP_Error.h"
#include "lib/pp/PP_Error_Private.h"
#include "lib/pp/traces/TraceConstantList.h"

#include "System/NetProtocol.h"
#include "Game/Game.h"
#include "Game/GameSetup.h"
#include "Sim/Misc/GlobalConstants.h" // needed for MAX_UNITS
#include "Sim/Misc/TeamHandler.h"
#include "Sim/Misc/LosHandler.h"
#include "Sim/Features/FeatureHandler.h"
#include "Sim/Features/FeatureSet.h"
#include "Sim/Units/Groups/GroupHandler.h"
#include "Sim/Units/Groups/Group.h"
#include "Sim/Units/CommandAI/Command.h"
#include "Sim/Units/CommandAI/CommandAI.h"
#include "Sim/Units/CommandAI/FactoryCAI.h"
#include "FileSystem/FileHandler.h"
#include "FileSystem/FileSystem.h"
#include "FileSystem/FileSystemHandler.h"
#include "GlobalUnsynced.h"
#include "LogOutput.h"
#include "Lua/LuaHandle.h"

const std::string archiveExpertPath = "traces\\expert\\";
const std::string archiveCompParamsPath = "traces\\params.json";
const std::string archiveFeedbacksPath = "traces\\feedbacks.xml";

const std::string springTracesPath = "traces\\";
const std::string springDataPath = "traces\\data\\";
const std::string springDefaultCompParamsPath = "traces\\data\\params.json";
const std::string springDefaultFeedbacksPath = "traces\\data\\feedbacks.xml";
const std::string springExpertPath = "traces\\data\\expert\\";
const std::string springLastFeedbacksPath = "traces\\last_Feedback.json";

const std::string server_name = "seriousgames.lip6.fr";
const std::string server_path = "/facebook/build_image.php";
const std::string server_images_path = "/facebook/images/generated/";

const std::string facebookUrl = "https://www.facebook.com/dialog/feed";
const std::string appId = "1199712723374964";
const std::string caption = "Essaye de faire mieux. Clique ici pour en savoir plus sur ProgAndPlay";
const std::string description = "ProgAndPlay est une bibliotheque de fonctions pour les jeux de Strategie Temps Reel (STR). Elle permet au joueur de programmer de maniere simple et interactive les entites virtuelles d'un STR. Actuellement...";
const std::string link = "https://www.irit.fr/ProgAndPlay/";
const std::string redirect_uri = "https://www.facebook.com/";

std::ofstream logFile("log.txt", std::ios::out | std::ofstream::trunc);
std::ofstream ppTraces;

void log(std::string msg) {
	logFile << msg << std::endl;
	logOutput.Print(msg.c_str());
}

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CProgAndPlay* pp;

CProgAndPlay::CProgAndPlay() : loaded(false), updated(false), missionEnded(false), traceModuleCorrectlyInitialized(false), tp(true), ta(), tracesComing(false), newExecutionDetected(false), endExecutionDetected(false), onGoingCompression(false) {
	log("ProgAndPLay constructor begin");

	// initialisation of Prog&Play
	if (PP_Init() == -1) {
		std::string tmp(PP_GetError());
		log(tmp.c_str());
	}
	else{
		log("Prog&Play initialized");
		loaded = true;
	}

	// The path where the .sdz are located
	archivePath = "mods\\" + archiveScanner->ArchiveFromName(gameSetup->modName);

  const std::map<std::string,std::string>& modOpts = gameSetup->modOptions;

	// init tracesOn depending on activetraces field in mod options
	bool tracesOn = modOpts.find("activetraces") != modOpts.end() && modOpts.at("activetraces").compare("1") == 0;

	// Check if we are in testing mode
	testMapMode = modOpts.find("testmap") != modOpts.end();

	if (modOpts.find("missionname") != modOpts.end())
		missionName = modOpts.at("missionname");
	else
		missionName = "";

	// init and open the appropriate traces file based on the current mission and launch the thread to perform traces
	if (tracesOn && missionName != ""){
		initTracesFile();
		if (traceModuleCorrectlyInitialized){
			log("launch thread to parse and compress traces");
			// thread creation
			std::string dirName = springTracesPath;
			dirName.erase(dirName.end()-1);
			tracesThread = boost::thread(&TracesParser::parseTraceFileOffline, &tp, dirName, missionName+".log");
		}
	}

	log("ProgAndPLay constructor end");
}

CProgAndPlay::~CProgAndPlay() {
  log("ProgAndPLay destructor begin");
  if (loaded){
	if (PP_Quit() == -1){
		std::string tmp(PP_GetError());
		log(tmp.c_str());
	}
	else
		log("Prog&Play shut down and cleaned up");
  }
  if (traceModuleCorrectlyInitialized) {
		if (!missionEnded) {
			// Stop thread
			tp.setEnd();
			tracesThread.join();
		}
		// Close traces file
		ppTraces.close();
		// then the trace module is no longer initialized
		traceModuleCorrectlyInitialized = false;
  }
  logFile.close();
  log("ProgAndPLay destructor end");
}

void CProgAndPlay::Update(void) {
	//log("ProgAndPLay::Update begin");

	// std::stringstream ss;

	// try to identify new pull of traces
	char * msg = PP_PopMessage();
	bool tracesComing = msg != NULL;
	// even if trace module is not activated, we pop messages by security in order to
	// avoid to fill shared memory in case of P&P client push traces whereas trace
	// module is not activated.
	while (msg != NULL) {
		std::string line (msg);
		if (traceModuleCorrectlyInitialized){
			if (line.find(EXECUTION_START_TIME) != std::string::npos){ // if we detect a new P&P program execution
				log("CProgAndPlay : new execution detected");
				newExecutionDetected = true;
				endExecutionDetected = false;
				units_idled_frame_counter = 0; // by setting units_idled_frame_counter to 0 we enable computing of units idle
			}
			// we send traces to the parser if traces received are included between an EXECUTION_START_TIME and EXECUTION_END_TIME events
			if (newExecutionDetected && !endExecutionDetected) {
				// we send this log to the parser
				if (missionEnded)
					ppTraces << DELAYED << " ";
				// Write this message on traces file
				ppTraces << msg << std::endl;
			}
			if (line.find(EXECUTION_END_TIME) != std::string::npos){ // if we detect the end of the P&P program execution
				log("CProgAndPlay : end execution detected");
				endExecutionDetected = true;
			}
		}
		// free memory storing this message
		delete[] msg;
		// Get next message
		msg = PP_PopMessage();
	}

	// Check if user asks help by UI (see pp_show_feedback.lua)
	bool askHelp = configHandler->GetString("helpPlease", "", true).compare("enabled") == 0;
	if (askHelp){
		log("CProgAndPlay : user ask help");
		configHandler->SetString("helpPlease", "", true); // reset help notifcation
	}

	// Check if mission is ended. This depends on engine state (game->gameOver) and/or
	// missions state (victoryState)
	if (!missionEnded) {
		std::string victoryState = configHandler->GetString("victoryState", "", true);
		if (victoryState.compare("") != 0 || game->gameOver) {
			// Log end messages
			if (traceModuleCorrectlyInitialized) {
				ppTraces << MISSION_END_TIME << " " << startTime + (int)std::floor(gu->PP_modGameTime) << std::endl;
				ppTraces << GAME_END << " " << victoryState << " " << missionName << std::endl;
			}
			log ("CProgAndPlay : missionEnded set to true");
			missionEnded = true;
		}
		mission_ended_frame_counter = 0; // reset mission ended counter
	} else{
		if (mission_ended_frame_counter > -1) // check if we have to compute mission ended
			mission_ended_frame_counter++;
	}
	// compute if we reach limits to accept end mission (usefull to take in account endless loop)
	if ((mission_ended_frame_counter-1)/GAME_SPEED < MISSION_ENDED_TRIGGER && mission_ended_frame_counter/GAME_SPEED >= MISSION_ENDED_TRIGGER)
		log("CProgAndPlay : mission ended limit time exceed");
	bool missionEndedReach = mission_ended_frame_counter/GAME_SPEED >= MISSION_ENDED_TRIGGER;

	if (traceModuleCorrectlyInitialized) {
		// check if all units are idled or destroyed
		bool unitsIdled = allUnitsIdled() || allUnitsDead();
		if (units_idled_frame_counter > -1){ // check if we have to compute units idle
			if (unitsIdled)
				units_idled_frame_counter++; // increase counter to know how much time all units are idled
			else
			  units_idled_frame_counter = 0; // reset units idle counter
		}
		// compute if we reach limits to accept idle units
		if ((units_idled_frame_counter - 1)/GAME_SPEED < UNITS_IDLED_TRIGGER && units_idled_frame_counter/GAME_SPEED >= UNITS_IDLED_TRIGGER)
			log("CProgAndPlay : units idle limit time exceed");
		bool unitsIdledReach = units_idled_frame_counter/GAME_SPEED >= UNITS_IDLED_TRIGGER;

		// Check if we have to start compression
		if (newExecutionDetected && (missionEndedReach || unitsIdledReach || askHelp) && !onGoingCompression){
			log("CProgAndPlay : ask parser to proceed and compress traces");
			// we ask trace parser to proceed all traces agregated from the last new execution event
			tp.setProceed(true);
			onGoingCompression = true;
		}
	}

	// Check if it's time to launch analysis of compressed traces
	if (onGoingCompression){
		if (tp.compressionDone()){
			// compression is done then proceed compression result by computing feedback
			// or storing expert solution
			log("CProgAndPlay : compression done");
			onGoingCompression = false;
			// if mission ended we stop thread. Indeed even if compression is done, the
			// thread is steal running. Then we explicitely ask to stop now
			if (missionEnded && traceModuleCorrectlyInitialized) {
				log("CProgAndPlay : turn off trace parser and set traceModuleCorrectlyInitialized to false");
				tp.setEnd();
				tracesThread.join();
				ppTraces.close();
				// then the trace module is no longer initialized
				traceModuleCorrectlyInitialized = false;
			}

			// If we are not in testing mode => build the feedback and send it to Lua context
			if (!testMapMode) {
				// inform trace analyser if we detect an endless loop
				ta.setEndlessLoop(tracesComing && !endExecutionDetected);
				// load expert xml solutions
				std::vector<std::string> experts_xml;
				std::vector<std::string> files = vfsHandler->GetFilesInDir(archiveExpertPath + missionName);
				for (unsigned int i = 0; i < files.size(); i++) {
					if (files.at(i).find(".xml") != std::string::npos && files.at(i).compare("feedbacks.xml") != 0)
						experts_xml.push_back(loadFileFromVfs(archiveExpertPath + missionName + "\\" + files.at(i)));
				}

				std::string feedback = "";
				if (!experts_xml.empty()) {
					log("CProgAndPlay : expert compressed traces found => compute feedbacks");
					// load learner xml solution
					const std::string learner_xml = loadFile(springTracesPath + missionName + "_compressed.xml");
					// compute feedbacks
					feedback = ta.constructFeedback(learner_xml, experts_xml, -1, -1);
					// Write into file
					std::ofstream jsonFile;
					jsonFile.open(springLastFeedbacksPath.c_str());
					if (jsonFile.good()) {
						jsonFile << feedback;
						jsonFile.close();
					}
				}
				else{
					log("CProgAndPlay : no expert compressed traces found => analysis aborted");
					// nothing more to do, we want to send an empty feedback
				}
				// Send feedback to Lua
				sendFeedback(feedback);

				// we get compressed trace that produce these feedbacks and send it to Lua
				std::string learner_compressed_txt = loadFile(springTracesPath + missionName + "_compressed.txt");
				// prefix message
				learner_compressed_txt.insert(0,"CompressedTraces_");
				std::vector<boost::uint8_t> data(learner_compressed_txt.size());
				std::copy(learner_compressed_txt.begin(), learner_compressed_txt.end(), data.begin());
				CLuaHandle::HandleLuaMsg(gu->myPlayerNum, LUA_HANDLE_ORDER_RULES, 0, data); // processed by mission_runner.lua
			} else {
				log("testmap mode: no analysis required, only compression");
				if (missionEnded) {
					// generating an expert solution for a mission
					// move traces and compressed traces files to directory 'traces\data\expert\missionName'
					bool dirExists = FileSystemHandler::mkdir(springDataPath);
					log("Try to create: " + springExpertPath);
					dirExists = FileSystemHandler::mkdir(springExpertPath);
					if (dirExists) {
						std::string path = springExpertPath + missionName;
						log("Try to create: " + path);
						dirExists = FileSystemHandler::mkdir(path);
						if (dirExists) {
							// renommage avec le plus grand entier non utilisé dans le repertoire
							int num = 1;
							DIR *pdir;
							struct dirent *pent;
							pdir = opendir(path.c_str());
							if (pdir) {
								while ((pent = readdir(pdir))) {
									std::string name = pent->d_name;
									if (name.find(".xml") != std::string::npos) {
										name.replace(name.find(".xml"), 4, "");
										int file_num = strtol(name.c_str(),NULL,10);
										if (file_num > 0)
											num = file_num + 1;
									}
								}
							}
							closedir(pdir);

							std::string oldName = springTracesPath + missionName + ".log";
							std::string newName = path + "\\" + boost::lexical_cast<std::string>(num) + ".log";
							if (rename(oldName.c_str(), newName.c_str()) == 0)
								log("raw traces successfully renamed");
							else
								log("raw traces rename operation failed");

							oldName = springTracesPath + missionName + "_compressed.xml";
							newName = path + "\\" + boost::lexical_cast<std::string>(num) + ".xml";
							if (rename(oldName.c_str(), newName.c_str()) == 0)
								log("compressed traces successfully renamed");
							else
								log("compressed traces rename operation failed");
						}
					}
				}
			}
		}
	} else {
		// No on going compression means no execution detected and so...
		// ...if player asks help we send him a feedback to ask him to execute a program first
		if (askHelp){
			log("CProgAndPlay : no on going compression but player ask help => send feedback to advise launching P&P program");
			// build a feedback formated te be interpreted by widget
			std::string feedback = "{\"feedbacks\": [\"Please execute a Prog&Play program before using help.\"]}";
			const std::map<std::string,std::string>& modOpts = gameSetup->modOptions;
			if (modOpts.find("language") != modOpts.end() && modOpts.at("language").compare("fr") == 0)
				feedback = "{\"feedbacks\": [\"Veuillez lancer un programme Prog&Play avant d'utiliser l'aide.\"]}";
			sendFeedback(feedback);
		}
		// ...if mission end exceeds the time limit we send and empty feedback to refresh UI. This occurs for exemple
		// when a player end a mission by end (without P&P program) in this case the Widget waiting feedback and we
		// send it an empty one
		if (missionEndedReach){
			log("CProgAndPlay : no on going compression but mission end => send an empty feedback");
			sendFeedback("");
		}
	}

	// check if the player has click on the publish tab. It can happen only if the mission is ended.
	if (configHandler->GetString("publish", "false", true).compare("true") == 0) {
		configHandler->SetString("publish", "false", true);
		publishOnFacebook();
	}

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

	//log("ProgAndPLay::Update end");
}

void CProgAndPlay::sendFeedback(std::string feedback){
	// Add prefix to json string
	feedback.insert(0,"Feedback_");
	// Send feedback to Lua (SendLuaRulesMsg function in LuaUnsyncedCtrl)
	std::vector<boost::uint8_t> data(feedback.size());
	std::copy(feedback.begin(), feedback.end(), data.begin());
	CLuaHandle::HandleLuaMsg(gu->myPlayerNum, LUA_HANDLE_ORDER_RULES, 0, data);// processed by mission_runner.lua
	// the feedback was sent the requirement of new feedback is satisfied
	units_idled_frame_counter = -1;
	mission_ended_frame_counter = -1;
	newExecutionDetected = false;
	endExecutionDetected = false;
}

/*
 * Returns the content of the file identified by full_path as a string
 */
std::string CProgAndPlay::loadFile(std::string full_path) {
	std::string res;
	std::ifstream in(full_path.c_str());
	if (in.good()) {
		std::string line;
		while(std::getline(in,line))
			res += line;
	}
	return res;
}

/*
 * Returns the content of the file located in the mod archive and identified by full_path as a string. The mod archive has to be loaded when the function is called.
 */
std::string CProgAndPlay::loadFileFromVfs(std::string full_path) {
	std::string res;
	std::vector<boost::uint8_t> data;
	if (vfsHandler->LoadFile(full_path, data))
		res.assign(data.begin(), data.end());
	return res;
}

void CProgAndPlay::GamePaused(bool paused) {
	int ret = PP_SetGamePaused(paused);
	if (ret == -1)
		return;
	if (traceModuleCorrectlyInitialized) {
		if (paused)
			ppTraces << GAME_PAUSED << std::endl;
		else
			ppTraces << GAME_UNPAUSED << std::endl;
	}
}

// this function is called in Game.cpp when the play starts
void CProgAndPlay::TracePlayer() {
	// if traceModuleCorrectlyInitialized is active (this means the trace module is enabled) => push
	// this value into the shared memory to enable trace in the client side
	if (traceModuleCorrectlyInitialized)
		PP_SetTracePlayer();
}

void CProgAndPlay::UpdateTimestamp() {
	PP_UpdateTimestamp(startTime + (int)std::floor(gu->PP_modGameTime));
}

PP_ShortUnit buildShortUnit(CUnit *unit, PP_Coalition c){
	//log("ProgAndPLay::buildShortUnit begin");
	PP_ShortUnit tmpUnit;
	tmpUnit.id = unit->id;
	tmpUnit.coalition = c;
	if (unit->unitDef)
		tmpUnit.type = unit->unitDef->id;
	else
		tmpUnit.type = -1;
	tmpUnit.pos.x = unit->pos.x;
	tmpUnit.pos.y = unit->pos.z;
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
				if (it->id != CMD_SET_WANTED_MAX_SPEED){
					PP_Command tmpCmd;
					tmpCmd.code = it->id;
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

	//log("ProgAndPLay::buildShortUnit end");
	return tmpUnit;
}

void freeShortUnit (PP_ShortUnit unit){
	//log("ProgAndPLay::freeShortUnit begin");
	if (unit.commandQueue != NULL){
		for (int i = 0 ; i < unit.nbCommandQueue ; i++){
			if (unit.commandQueue[i].param != NULL)
				free(unit.commandQueue[i].param);
		}
		free (unit.commandQueue);
		unit.commandQueue = NULL;
		unit.nbCommandQueue = 0;
	}
	//log("ProgAndPLay::freeShortUnit end");
}

void doUpdate(CUnit* unit, PP_Coalition c){
	//std::stringstream str;
	//str << "ProgAndPLay::doUpdate begin : " << unit->id;
	//log(str.str());
	PP_ShortUnit shortUnit = buildShortUnit(unit, c);
	if (PP_UpdateUnit(shortUnit) == -1){
		std::string tmp(PP_GetError());
		log(tmp.c_str());
	}
	freeShortUnit(shortUnit);
	//log("ProgAndPLay::doUpdate end");
}

void doAdding(CUnit* unit, PP_Coalition c){
	//std::stringstream str;
	//str << "ProgAndPLay::doAdding begin : " << unit->id;
	//log(str.str());
	PP_ShortUnit shortUnit = buildShortUnit(unit, c);
	if (PP_AddUnit(shortUnit) == -1){
		std::string tmp(PP_GetError());
		log(tmp.c_str());
	}
	freeShortUnit(shortUnit);
	//log("ProgAndPLay::doAdding end");
}

void doRemoving (int unitId){
	//std::stringstream ss;
	//ss << "ProgAndPLay::doRemoving begin : " << unitId;
	//log(ss.str());
	if (PP_RemoveUnit(unitId) == -1){
		std::string tmp(PP_GetError());
		log(tmp.c_str());
	}
	//log("ProgAndPLay::doRemoving end");
}

void CProgAndPlay::AddUnit(CUnit* unit){
	//log("ProgAndPLay::AddUnit begin");
	PP_Coalition c;
	if (unit->team == gu->myTeam)
		c = MY_COALITION;
	else if (teamHandler->AlliedTeams(gu->myTeam, unit->team))
		c = ALLY_COALITION;
	else
		c = ENEMY_COALITION;
	if (c != ENEMY_COALITION || loshandler->InLos(unit, gu->myAllyTeam)){
		doAdding(unit, c);
	}
	//log("ProgAndPLay::AddUnit end");
}

void CProgAndPlay::UpdateUnit(CUnit* unit){
	//log("ProgAndPLay::UpdateUnit begin");
	PP_Coalition c;
	if (unit->team == gu->myTeam)
		c = MY_COALITION;
	else if (teamHandler->AlliedTeams(gu->myTeam, unit->team))
		c = ALLY_COALITION;
	else
		c = ENEMY_COALITION;
	if (c == MY_COALITION){
		doUpdate(unit, c);
	}
	else {
		int isStored = PP_IsStored(unit->id);
		if (isStored == -1){
			std::string tmp(PP_GetError());
			log(tmp.c_str());
		}
		else {
			if (loshandler->InLos(unit, gu->myAllyTeam)){
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
	//log("ProgAndPLay::UpdateUnit end");
}

void CProgAndPlay::RemoveUnit(CUnit* unit){
	//log("ProgAndPLay::RemoveUnit begin");
	doRemoving(unit->id);
	//log("ProgAndPLay::RemoveUnit end");
}

int CProgAndPlay::updatePP(void){
	//log("ProgAndPLay::updatePP begin");
	if (!updated) {
		updated = true;
		// store map size
		PP_Pos mapSize;
		mapSize.x = gs->mapx*SQUARE_SIZE;
		mapSize.y = gs->mapy*SQUARE_SIZE;
		// store starting position
		PP_Pos startPos;
		startPos.x = teamHandler->Team(gu->myTeam)->startPos.x;
		startPos.y = teamHandler->Team(gu->myTeam)->startPos.z;
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
	resources.resource[0] = teamHandler->Team(gu->myTeam)->metal;
	resources.resource[1] = teamHandler->Team(gu->myTeam)->energy;
	int ret = PP_SetRessources(resources);
	free(resources.resource);
	if (ret == -1)
		return -1;

	// set game over
	ret = PP_SetGameOver(missionEnded);
	//delete tmpFile;
	if (ret == -1)
		return -1;

	//log("ProgAndPLay::updatePP end");
	return 0;
}

int CProgAndPlay::execPendingCommands(){
	//log("ProgAndPLay::execPendingCommands begin");
	int nbCmd = 0;
	PP_PendingCmds* pendingCommands = PP_GetPendingCommands();
	if (pendingCommands == NULL) return -1;
	else{
		nbCmd = pendingCommands->size;
		for (int i = 0 ; i < pendingCommands->size ; i++){
			PP_PendingCmd pc = pendingCommands->pendingCommand[i];
			// Check for affecting group
			if (pc.group != -2){
				// Check if unit exists
				CUnit* tmp = uh->GetUnit(pc.unitId);
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
						ground->GetHeight(pc.command.param[0], pc.command.param[2]);
					break;
				case 1 : // the command targets a unit
					// Find targeted unit
					for (int t = 0 ; t < teamHandler->ActiveTeams() && !found ; t++){
						CUnit* tmp = uh->GetUnit((int)pc.command.param[0]);
						if (tmp != NULL){
							if (tmp->team == t){
								// check if this unit is visible by the player
								if (!loshandler->InLos(tmp, gu->myAllyTeam))
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
					CUnit* fact = uh->GetUnit(pc.unitId);
					if (fact){
						CFactoryCAI* facAI = dynamic_cast<CFactoryCAI*>(fact->commandAI);
						if (facAI) {
							Command c;
							c.options = RIGHT_MOUSE_KEY; // Set RIGHT_MOUSE_KEY option in order to decrement building order
							CCommandQueue& buildCommands = facAI->commandQue;
							CCommandQueue::iterator it;
							std::vector<Command> clearCommands;
							clearCommands.reserve(buildCommands.size());
							for (it = buildCommands.begin(); it != buildCommands.end(); ++it) {
								c.id = it->id;
								clearCommands.push_back(c);
							}
							for (int i = 0; i < (int)clearCommands.size(); i++) {
								facAI->GiveCommand(clearCommands[i]);
							}
						}
					}
				} else{
					// Copy pc.command.param in a vector to pass it at SendAICommand
					std::vector<float> stdParam;
					for (int j = 0 ; j < pc.command.nbParam ; j++)
						stdParam.push_back(pc.command.param[j]);
					net->Send(CBaseNetProtocol::Get().SendAICommand(gu->myPlayerNum, pc.unitId,
						pc.command.code, 0, stdParam));
/*					// Permet de gérer les commandes plus correctement mais produit des
					// erreurs de synchronisation (à utiliser en SYNCED_MODE)
					CUnitSet *tmp = &(gs->Team(gu->myTeam)->units);
					CUnitSetConstIterator it = tmp->find(pc.unitId);
					if (it != tmp->end() && (*it)->commandAI){
						Command cTmp;
						cTmp.id = pc.command.code;
						cTmp.params = stdParam;
						(*it)->commandAI->GiveCommand(cTmp);
					}*/
				}
			}
		}
	}
	PP_FreePendingCommands(pendingCommands);
	//log("ProgAndPLay::execPendingCommands end");
	return nbCmd;
}

bool CProgAndPlay::allUnitsIdled() {
	CUnitSet *tmp = &(teamHandler->Team(gu->myTeam)->units);
	if (tmp->size() == 0)
		return false;
	CUnitSet::iterator it = tmp->begin();
	while (it != tmp->end()) {
		const CCommandAI* commandAI = (*it)->commandAI;
		if (commandAI != NULL) {
			const CCommandQueue* queue = &commandAI->commandQue;
			if (queue->size() > 0)
				return false;
		}
		it++;
	}
	return true;
}

bool CProgAndPlay::allUnitsDead() {
	CUnitSet *tmp = &(teamHandler->Team(gu->myTeam)->units);
	return tmp->size() == 0;
}

void CProgAndPlay::initTracesFile() {
	log("CProgAndPLay::initTracesFile begin");
	const std::map<std::string,std::string>& modOpts = gameSetup->modOptions;
	// build a directory to store traces
	bool dirExists = FileSystemHandler::mkdir(springTracesPath);
	if (dirExists) {
		bool defaultFeedbacksFound = false;
		// Check if we are in testing mode
		if (testMapMode){
			// try to found local compression parameters (json file) in mission associated folder
			TracesParser::params_json = loadFile(springExpertPath + missionName + "\\params.json");
			if (TracesParser::params_json.compare("") != 0)
				log("Specific mission compression params loaded");
			else {
				// try to found default compression parameters included into spring engine directory
				TracesParser::params_json = loadFile(springDefaultCompParamsPath);
				if (TracesParser::params_json.compare("") != 0)
					log("Default compression params loaded from spring directory");
				else
					log("No compression params found builtin compression params will be used");
			}
			// No need to load feedbacks in testing mode
			log("Test mission in editor mode => no traces analysis only compression");
		} else {
			// try to found local compression parameters (json file) in mission associated folder
			TracesParser::params_json = loadFileFromVfs(archiveExpertPath + missionName + "\\params.json");
			if (TracesParser::params_json.compare("") != 0)
				log("Specific mission compression params loaded");
			else {
				// try to found default compression parameters included into mod archive
				TracesParser::params_json = loadFileFromVfs(archiveCompParamsPath);
				if (TracesParser::params_json.compare("") != 0)
					log("Default compression params loaded from mod archive");
				else{
					// try to found default compression parameters included into spring engine directory
					TracesParser::params_json = loadFile(springDefaultCompParamsPath);
					if (TracesParser::params_json.compare("") != 0)
						log("Default compression params loaded from spring directory");
					else
						log("No compression params found built-in compression params will be used");
				}
			}
			// try to found local feedbacks from the mission tested
			std::string mission_feedbacks_xml;
			// We try to found a local feedback file for the loaded mission
			// By convention only one file of this kind is included into the mission directory
			std::vector<std::string> files = vfsHandler->GetFilesInDir(archiveExpertPath + missionName);
			for (unsigned int i = 0; i < files.size(); i++) {
				if (files.at(i).compare("feedbacks.xml") == 0) {
					log("Specific mission feedbacks loaded from mod archive");
					mission_feedbacks_xml = loadFileFromVfs(archiveExpertPath + missionName + "\\" + files.at(i));
				}
			}
			// We load the global feedback file, first we try to find it in mods archive
			std::string feedbacks_xml = loadFileFromVfs(archiveFeedbacksPath);
			if (feedbacks_xml.compare("") != 0){
				log("Default feedbacks loaded from mod archive");
				defaultFeedbacksFound = true;
			}
			else{
				// try to found default feedbacks included into spring engine directory
				feedbacks_xml = loadFile(springDefaultFeedbacksPath);
				if (feedbacks_xml.compare("") != 0){
					log("Default feedbacks loaded from spring directory");
					defaultFeedbacksFound = true;
				}
				else
					log("WARNING!!! No default feedbacks are available => no analysis will be possible. Check your installation...");
			}
			// If global feedback file is defined we push it to trace analyser
			if (defaultFeedbacksFound){
				// defining langage for the analyser
				ta.setLang((modOpts.find("language") != modOpts.end()) ? modOpts.at("language") : "en");
				ta.loadXmlInfos(feedbacks_xml,mission_feedbacks_xml);
				log("trace analyser initialized (language and feedbacks)");
			}
		}

		// create a log file to store traces for this mission
		std::stringstream ss;
		ss << springTracesPath << missionName << ".log";
		ppTraces.open(ss.str().c_str(), std::ios::out | std::ios::app | std::ios::ate);
		if (ppTraces.is_open() && (testMapMode || defaultFeedbacksFound)) {
			// mod options enables traces and trace file is openned => we consider that the trace module
			// is properly initialized, then we set traceModuleCorrectlyInitialized to true
			traceModuleCorrectlyInitialized = true;
			startTime = std::time(NULL);
			// init trace with first logs
			ppTraces << GAME_START << " " << missionName << std::endl;
			ppTraces << MISSION_START_TIME << " " << startTime << std::endl;
		}
	}
	else {
		PP_SetError("ProgAndPlay::cannot create traces directory");
		log(PP_GetError());
	}
	log("CProgAndPLay::initTracesFile end");
}

// Publish on facebook functions

void CProgAndPlay::openFacebookUrl() {
	std::string url = facebookUrl+"?app_id="+appId+"&display=page&caption="+caption+"&description="+description+"&link="+link+"&href="+link+"&redirect_uri="+redirect_uri+"&picture="+server_name+server_images_path+photoFilename;
	#ifdef __linux__
		std::string cmd = "x-www-browser " + url;
		system(cmd.c_str());
	#elif _WIN32
		ShellExecuteA(NULL, "open", url.c_str(), NULL, NULL, SW_SHOWNORMAL);
	#endif
}

void CProgAndPlay::sendRequestToServer() {
	using boost::asio::ip::tcp;
	try {
		boost::asio::io_service io_service;

		// Get a list of endpoints corresponding to the server name.
		tcp::resolver resolver(io_service);
		tcp::resolver::query query(server_name, "http");
		tcp::resolver::iterator endpoint_iterator = resolver.resolve(query);
		tcp::resolver::iterator end;

		// Try each endpoint until we successfully establish a connection.
		tcp::socket socket(io_service);
		boost::system::error_code error = boost::asio::error::host_not_found;
		while (error && endpoint_iterator != end) {
			socket.close();
			socket.connect(*endpoint_iterator++, error);
		}
		if (error)
			throw boost::system::system_error(error);

		boost::asio::streambuf request;
		std::ostream request_stream(&request);
		request_stream << "GET " << server_path << "?score=" << configHandler->GetString("score","", true) << "&mission_name=" << missionName << " HTTP/1.0\r\n";
		request_stream << "Host: " << server_name << "\r\n";
		request_stream << "Accept: */*\r\n";
		request_stream << "Connection: close\r\n\r\n";

		boost::asio::write(socket, request);

		// Read the response status line. The response streambuf will automatically
		// grow to accommodate the entire line. The growth may be limited by passing
		// a maximum size to the streambuf constructor.
		boost::asio::streambuf response;
		boost::asio::read_until(socket, response, "\r\n");

		// Check that response is OK.
		std::istream response_stream(&response);
		std::string http_version;
		response_stream >> http_version;
		unsigned int status_code;
		response_stream >> status_code;
		std::string status_message;
		std::getline(response_stream, status_message);
		if (!response_stream || http_version.substr(0, 5) != "HTTP/") {
			log("Invalid response\n");
			return;
		}
		if (status_code != 200) {
			std::ostringstream oss;
			oss << "Response returned with status code " << status_code << "\n";
			log(oss.str());
			return;
		}
		// Read the response headers, which are terminated by a blank line.
		boost::asio::read_until(socket, response, "\r\n\r\n");
		// Process the response headers.
		std::string header;
		while (std::getline(response_stream, header) && header != "\r");
		// Write whatever content we already have to output.
		if (response.size() > 0) {
			std::ostringstream oss;
			oss << &response;
			photoFilename = oss.str();
		}
		// Read until EOF, writing data to output as we go.
		while (boost::asio::read(socket, response, boost::asio::transfer_at_least(1), error))
			std::cout << &response;
		if (error != boost::asio::error::eof)
			throw boost::system::system_error(error);
	}
	catch (std::exception& e) {
		std::ostringstream oss;
		oss << "Exception: " << e.what() << "\n";
		log(oss.str());
	}
}

void CProgAndPlay::publishOnFacebook() {
	log("publish");
	if (photoFilename.empty() && !configHandler->GetString("score", "", true).empty()) {
		log("sending request to the server");
		sendRequestToServer();
	}
	if (!photoFilename.empty()) {
		log("open facebook url");
		openFacebookUrl();
	}
}

// ---
