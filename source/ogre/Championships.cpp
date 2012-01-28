#include "pch.h"
#include "Defines.h"
#include "../vdrift/pathmanager.h"
#include "../vdrift/game.h"
#include "OgreGame.h"

using namespace std;
using namespace Ogre;
using namespace MyGUI;


///  Load  championships.xml, progress.xml (once)
//---------------------------------------------------------------------
void App::ChampsXmlLoad()
{
	champs.LoadXml(PATHMANAGER::GetGameConfigDir() + "/championships.xml");
	LogO(String("**** Loaded Championships: ") + toStr(champs.champs.size()));
	
	progress.LoadXml(PATHMANAGER::GetUserConfigDir() + "/progress.xml");

	int pcs = progress.champs.size(), chs = champs.champs.size();
	//  progress empty, fill with 0s
	if (pcs == 0)
	{
		LogO(String("|| No progress found - saving empty with 0s."));
		for (int i=0; i < chs; ++i)
		{
			const Champ& ch = champs.champs[i];
			ProgressChamp pc;
			pc.name = ch.name;  // save for ver check
			pc.ver = ch.ver;
			ProgressTrack pt;  // empty, 0 progress
			for (int t=0; t < champs.champs[i].trks.size(); ++t)
				pc.trks.push_back(pt);
			progress.champs.push_back(pc);
		}
		ProgressSave(false);  //will be later in guiInit
	}else
	{
		if (pcs != chs)
		{
			// add new champs to progress ...
			//for (int i=0; i < chs; ++i)
		}
		//  check ver, name, trks size, if different reset that champ only ...
		//for (int i=0; i < pcs; ++i)
	}
}

///  load championship track
//---------------------------------------------------------------------
void App::ChampNewGame()
{
	if (pSet->game.champ_num >= champs.champs.size())
		pSet->game.champ_num = -1;  //0 range

	int chId = pSet->game.champ_num;
	if (chId >= 0)
	{
		//  champ stage, current track
		const ProgressChamp& pc = progress.champs[chId];
		const Champ& ch = champs.champs[chId];
		const ChampTrack& trk = ch.trks[pc.curTrack];
		pSet->game.track = trk.name;
		pSet->game.track_user = 0;
		pSet->game.trackreverse = trk.reversed;
		pSet->game.num_laps = trk.laps;

		pSet->game.boost_type = 1;  // from trk.?
		pSet->game.flip_type = 1;
		pSet->game.boost_power = 1.f;
		//pSet->game.trees = 1.f;  // >=1 ?
		//pSet->game.collis_veget = true;

		pGame->pause = true;  // wait for stage wnd close
		pGame->timer.waiting = true;
	}else
	{	pGame->pause = false;  // single race
		pGame->timer.waiting = false;
	}
}

///  Championships list  sel changed
//---------------------------------------------------------------------
void App::ChampsListUpdate()
{
	const char clrT[3][8] = {"#A0F0FF", "#FFFFB0", "#FFA0A0"};
	const static char clrDiff[8][8] =  // track difficulty colors
		{"#60C0FF", "#00FF00", "#60FF00", "#C0FF00", "#FFFF00", "#FFC000", "#FF6000", "#FF4040"};

	liChamps->removeAllItems();  char ss[64];
	for (int i=0; i < champs.champs.size(); ++i)
	{
		const Champ& ch = champs.champs[i];
		const ProgressChamp& pc = progress.champs[i];
		int ntrks = pc.trks.size();
		const String& clr = clrT[ch.tutorial];
		liChamps->addItem(toStr(i/10)+toStr(i%10), 0);  int l = liChamps->getItemCount()-1;
		liChamps->setSubItemNameAt(1,l, clr+ ch.name.c_str());
		liChamps->setSubItemNameAt(2,l, clrDiff[ch.diff]+ TR("#{Diff"+toStr(ch.diff)+"}"));
		liChamps->setSubItemNameAt(3,l, clrDiff[std::min(7,ntrks*2/3+1)]+ toStr(ntrks));
		sprintf(ss, "%3.0f %%", 100.f * pc.curTrack / ntrks);
		liChamps->setSubItemNameAt(4,l, clr+ ss);
		sprintf(ss, "%5.1f", pc.score);
		liChamps->setSubItemNameAt(5,l, clr+ ss);
		//length,time;
	}
	liChamps->setIndexSelected(pSet->gui.champ_num);  //range..
}

///  Championships list  sel changed
//---------------------------------------------------------------------
void App::listChampChng(MyGUI::MultiListBox* chlist, size_t pos)
{
	if (pos < 0)  return;
	if (pos >= champs.champs.size())  {  LogO("Error champ sel > size.");  return;  }
	//if (pos >= progress.champs.size())  {  LogO("Error progres sel > size.");  return;  }
	
	//  update champ stages
	liStages->removeAllItems();  char ss[64];
	const Champ& ch = champs.champs[pos];
	for (int i=0; i < ch.trks.size(); ++i)
	{
		const ChampTrack& trk = ch.trks[i];
		liStages->addItem(toStr(i/10)+toStr(i%10), 0);  int l = liStages->getItemCount()-1;
		liStages->setSubItemNameAt(1,l, trk.name.c_str());

		int id = tracksXml.trkmap[trk.name];
		const TrackInfo& ti = tracksXml.trks[id-1];

		liStages->setSubItemNameAt(2,l, ti.scenery);
		liStages->setSubItemNameAt(3,l, toStr(ti.diff));

		liStages->setSubItemNameAt(4,l, toStr(trk.laps));
		sprintf(ss, "%5.1f", progress.champs[pos].trks[i].score);
		liStages->setSubItemNameAt(5,l, ss);
	}
	//  descr
	EditBox* ed = mGUI->findWidget<EditBox>("ChampDescr");
	if (ed)  ed->setCaption(ch.descr);

	//  update champ details (on stages tab)
	TextBox* txt;
	txt = mGUI->findWidget<TextBox>("valChDiff");
	if (txt)  txt->setCaption(TR("#{Diff"+toStr(ch.diff)+"}"));
	txt = mGUI->findWidget<TextBox>("valChTracks");
	if (txt)  txt->setCaption(toStr(ch.trks.size()));

	txt = mGUI->findWidget<TextBox>("valChDist");
	if (txt)  txt->setCaption(toStr(ch.length));  // sum from find tracks..
	txt = mGUI->findWidget<TextBox>("valChTime");
	if (txt)  txt->setCaption(toStr(ch.time));    // sum champs.trkTimes..

	txt = mGUI->findWidget<TextBox>("valChProgress");
	sprintf(ss, "%5.1f %%", 100.f * progress.champs[pos].curTrack / champs.champs[pos].trks.size());
	if (txt)  txt->setCaption(ss);
	txt = mGUI->findWidget<TextBox>("valChScore");
	sprintf(ss, "%5.1f", progress.champs[pos].score);
	if (txt)  txt->setCaption(ss);
}
//---------------------------------------------------------------------


///  champ start  -----
void App::btnChampStart(WP)
{
	pSet->gui.champ_num = liChamps->getIndexSelected();
	LogO("|| Starting champ: "+toStr(pSet->gui.champ_num));

	//  if already finished, restart - will loose progress and scores ..
	int chId = pSet->gui.champ_num;
	ProgressChamp& pc = progress.champs[chId];
	if (pc.curTrack == pc.trks.size())
	{
		LogO("|| Was at 100%, restarting progress.");
		pc.curTrack = 0;
		//pc.score = 0.f;
	}

	btnNewGame(0);
}

//  stage back
void App::btnChampStageBack(WP)
{
	mWndChampStage->setVisible(false);
	isFocGui = true;  // show back gui
	toggleGui(false);
}

///  stage start / end
//---------------------------------------------------------------------
void App::btnChampStageStart(WP)
{
	//  check if champ ended
	int chId = pSet->game.champ_num;
	ProgressChamp& pc = progress.champs[chId];
	const Champ& ch = champs.champs[chId];
	bool last = pc.curTrack == ch.trks.size();
	LogO("|| This was stage 2 close" + toStr(pc.curTrack) + "/" + toStr(ch.trks.size()));
	if (last)
	{	//  show end window
		mWndChampStage->setVisible(false);
		mWndChampEnd->setVisible(true);
		return;
	}

	bool finished = pGame->timer.GetLastLap() > 0.f;  //?-
	if (finished)
	{
		LogO("|| Loading next stage: "/* + ch.trks[pc.curTrack].name*/);
		mWndChampStage->setVisible(false);
		btnNewGame(0);
	}else
	{
		LogO("|| Starting stage.");
		mWndChampStage->setVisible(false);
		pGame->pause = false;
		pGame->timer.waiting = false;
	}
}

//  champ end
void App::btnChampEndClose(WP)
{
	mWndChampEnd->setVisible(false);
}

//  stage loaded
void App::ChampLoadEnd()
{
	if (pSet->game.champ_num >= 0)
	{
		ChampFillStageInfo(false);
		mWndChampStage->setVisible(true);
	}
}

///  save progress and update it on gui
void App::ProgressSave(bool upgGui)
{
	progress.SaveXml(PATHMANAGER::GetUserConfigDir() + "/progress.xml");
	if (!upgGui)
		return;
	ChampsListUpdate();
	listChampChng(liChamps, liChamps->getIndexSelected());
}


///  championship advance logic
//---------------------------------------------------------------------
void App::ChampionshipAdvance(float timeCur)
{
	int chId = pSet->game.champ_num;
	ProgressChamp& pc = progress.champs[chId];
	const Champ& ch = champs.champs[chId];
	const ChampTrack& trk = ch.trks[pc.curTrack];
	LogO("|| --- Champ end: " + ch.name);

	///  compute track :score:  --------------
	float timeBest = champs.trkTimes[trk.name];
	if (timeBest < 1.f)
	{	LogO("|| Error: Track has no best time !");  timeBest = 10.f;	}
	timeBest *= trk.laps;
	timeBest += 2;  // first lap longer, time at start spent to gain car valocity
	float factor = ch.trks[pc.curTrack].factor;  // how close to best you need to be
	timeBest *= 1.0f - factor;

	LogO("|| Track: " + trk.name);
	LogO("|| Your time: " + toStr(timeCur));
	LogO("|| Best time: " + toStr(timeBest));

	float score = timeCur / timeBest * 100.f;	//(timeBest-timeCur)/timeBest * 100.f;  //-
	LogO("|| Score: " + toStr(score));
	pc.trks[pc.curTrack].score = score;

	//  --------------  advance  --------------
	bool last = pc.curTrack+1 == ch.trks.size();
	LogO("|| This was stage " + toStr(pc.curTrack+1) + "/" + toStr(ch.trks.size()));
	if (!last)
	{
		//  show stage end [window]
		pGame->pause = true;
		pGame->timer.waiting = true;

		ChampFillStageInfo(true);  // cur track
		mWndChampStage->setVisible(true);
		
		pc.curTrack++;  // next stage
		ProgressSave();
	}else
	{
		//  champ ended
		pGame->pause = true;
		pGame->timer.waiting = true;

		ChampFillStageInfo(true);  // cur track
		mWndChampStage->setVisible(true);

		///  compute champ :score:  --------------
		int ntrk = pc.trks.size();  float sum = 0.f;
		for (int t=0; t < ntrk; ++t)
			sum += pc.trks[t].score;

		pc.curTrack++;  // end = 100 %
		pc.score = sum / ntrk;  // average from all tracks
		ProgressSave();

		LogO("|| Champ finished");
		LogO("|| Total score: " + toStr(score));  //..
		
		//  upd champ end [window]
		char ss[64];
		sprintf(ss, "%5.1f", score);
		String s = "Championship: " + ch.name + "\n" +
			"Total score: " + ss;
		edChampEnd->setCaption(s);
		//mWndChampEnd->setVisible(true);  // show after stage end
	}
}

void App::ChampFillStageInfo(bool finished)
{
	int chId = pSet->game.champ_num;
	ProgressChamp& pc = progress.champs[chId];
	const Champ& ch = champs.champs[chId];
	const std::string& trkName = ch.trks[pc.curTrack].name;

	String s;  char ss[64];
	s = "Championship: " + ch.name + "\n" +
		"Stage: " + toStr(pc.curTrack+1) + "/" + toStr(ch.trks.size()) + "\n" +
		"Track: " + trkName + "\n\n" /*+
		"Difficulty: " + tracksXml. + "\n"*/;

	if (finished)
	{
		sprintf(ss, "%5.1f", pc.trks[pc.curTrack].score);
		s += String("Finished.\n") +
			"Score: " + ss + "\n";
	}
	edChampStage->setCaption(s);
	
	imgChampStage->setImageTexture(trkName+".jpg");
}
