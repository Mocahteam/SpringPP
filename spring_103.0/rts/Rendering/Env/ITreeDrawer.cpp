/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */


#include "ITreeDrawer.h"
#include "BasicTreeDrawer.h"
#include "AdvTreeDrawer.h"
#include "Map/ReadMap.h"
#include "Rendering/GlobalRendering.h"
#include "Sim/Features/FeatureHandler.h"
#include "Sim/Features/Feature.h"
#include "Sim/Features/FeatureDef.h"
#include "Sim/Misc/GlobalConstants.h"
#include "System/Config/ConfigHandler.h"
#include "System/EventHandler.h"
#include "System/Exceptions.h"
#include "System/Log/ILog.h"
#include "System/myMath.h"
#include "System/Util.h"

CONFIG(int, TreeRadius)
	.defaultValue((int) (5.5f * 256))
	.headlessValue(0)
	.minimumValue(0);

CONFIG(bool, 3DTrees).defaultValue(true).headlessValue(false).safemodeValue(false).description("Defines whether or not the trees generated by the engine (Default trees) will be shown as 3d or as cross sectioned ( + ) flat sides.");

ITreeDrawer* treeDrawer = NULL;


ITreeDrawer::ITreeDrawer()
	: CEventClient("[ITreeDrawer]", 314444, false), drawTrees(true)
{
	eventHandler.AddClient(this);
	baseTreeDistance = configHandler->GetInt("TreeRadius") / 256.0f;

	treesX = mapDims.mapx / TREE_SQUARE_SIZE;
	treesY = mapDims.mapy / TREE_SQUARE_SIZE;
	nTrees = treesX * treesY;
}

ITreeDrawer::~ITreeDrawer() {
	eventHandler.RemoveClient(this);
	configHandler->Set("TreeRadius", (unsigned int) (baseTreeDistance * 256));
}



void ITreeDrawer::AddTrees()
{
	const CFeatureSet& features = featureHandler->GetActiveFeatures();

	for (CFeatureSet::const_iterator it = features.begin(); it != features.end(); ++it) {
		const CFeature* f = *it;

		if (f->def->drawType >= DRAWTYPE_TREE) {
			AddTree(f->id, f->def->drawType - 1, f->pos, 1.0f);
		}
	}
}


void ITreeDrawer::AddTree(int treeID, int treeType, const float3& pos, float size)
{
	TreeStruct ts;
	ts.id = treeID;
	ts.type = treeType;
	ts.pos = pos;

	const int treeSquareSize = SQUARE_SIZE * TREE_SQUARE_SIZE;
	const int treeSquareIdx =
		(((int)pos.x) / (treeSquareSize)) +
		(((int)pos.z) / (treeSquareSize) * treesX);

	VectorInsertUnique(treeSquares[treeSquareIdx].trees, ts, true);
	ResetPos(pos);
}

void ITreeDrawer::DeleteTree(int treeID, const float3& pos)
{
	const int treeSquareSize = SQUARE_SIZE * TREE_SQUARE_SIZE;
	const int treeSquareIdx =
		(((int)pos.x / (treeSquareSize))) +
		(((int)pos.z / (treeSquareSize) * treesX));

	VectorEraseIf(treeSquares[treeSquareIdx].trees, [treeID](const TreeStruct& ts) { return (treeID == ts.id); });
	ResetPos(pos);
}


void ITreeDrawer::ResetPos(const float3& pos)
{
	const int x = (int)(pos.x / TREE_SQUARE_SIZE / SQUARE_SIZE);
	const int y = (int)(pos.z / TREE_SQUARE_SIZE / SQUARE_SIZE);

	TreeSquareStruct& tss = treeSquares[y * treesX + x];

	if (tss.dispList) {
		delDispLists.push_back(tss.dispList);
		tss.dispList = 0;
	}
	if (tss.farDispList) {
		delDispLists.push_back(tss.farDispList);
		tss.farDispList = 0;
	}
}


ITreeDrawer* ITreeDrawer::GetTreeDrawer()
{
	ITreeDrawer* td = NULL;

	try {
		if (configHandler->GetBool("3DTrees")) {
			td = new CAdvTreeDrawer();
		}
	} catch (const content_error& e) {
		if (e.what()[0] != '\0') {
			LOG_L(L_ERROR, "%s", e.what());
		}
		LOG("TreeDrawer: Fallback to BasicTreeDrawer.");
		// td can not be != NULL here
		//delete td;
	}

	if (!td) {
		td = new CBasicTreeDrawer();
	}

	td->AddTrees();

	return td;
}



void ITreeDrawer::DrawShadowPass()
{
}

void ITreeDrawer::Draw(bool drawReflection)
{
	const float maxDistance = CGlobalRendering::MAX_VIEW_RANGE / (SQUARE_SIZE * TREE_SQUARE_SIZE);
	const float treeDistance = Clamp(baseTreeDistance, 1.0f, maxDistance);
	// call the subclasses draw method
	Draw(treeDistance, drawReflection);
}

void ITreeDrawer::Update()
{
	std::vector<GLuint>::iterator i;
	for (i = delDispLists.begin(); i != delDispLists.end(); ++i) {
		glDeleteLists(*i, 1);
	}
	delDispLists.clear();
}



void ITreeDrawer::RenderFeatureCreated(const CFeature* feature) {
	// support /give'ing tree objects
	if (feature->def->drawType >= DRAWTYPE_TREE) {
		AddTree(feature->id, feature->def->drawType - 1, feature->pos, 1.0f);
	}
}

void ITreeDrawer::FeatureMoved(const CFeature* feature, const float3& oldpos) {
	if (feature->def->drawType >= DRAWTYPE_TREE) {
		DeleteTree(feature->id, oldpos);
		AddTree(feature->id, feature->def->drawType - 1, feature->pos, 1.0f);
	}
}

void ITreeDrawer::RenderFeatureDestroyed(const CFeature* feature) {
	if (feature->def->drawType >= DRAWTYPE_TREE) {
		DeleteTree(feature->id, feature->pos);

		if (feature->speed.SqLength2D() > 0.25f) {
			AddFallingTree(feature->id, feature->def->drawType - 1, feature->pos, feature->speed * XZVector);
		}
	}
}
