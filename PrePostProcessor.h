/*
 *  This file is part of nzbget
 *
 *  Copyright (C) 2007  Andrei Prygounkov <hugbug@users.sourceforge.net>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * $Revision$
 * $Date$
 *
 */


#ifndef PREPOSTPROCESSOR_H
#define PREPOSTPROCESSOR_H

#include <deque>

#include "Thread.h"
#include "Observer.h"
#include "DownloadInfo.h"

#ifndef DISABLE_PARCHECK
#include "ParChecker.h"
#endif

class PrePostProcessor : public Thread
{
public:

	enum EPostJobStage
	{
		ptQueued,
		ptLoadingPars,
		ptVerifyingSources,
		ptRepairing,
		ptVerifyingRepaired,
		ptExecutingScript,
		ptFinished
	};

	class PostJob
	{
	private:
		char*			m_szNZBFilename;
		char*			m_szDestDir;
		char*			m_szParFilename;
		char*			m_szInfoName;
		bool			m_bWorking;
		bool			m_bParCheck;
		int				m_iParStatus;
		bool			m_bParFailed;
		EPostJobStage	m_eStage;
		char*			m_szProgressLabel;
		int				m_iFileProgress;
		int				m_iStageProgress;
		time_t			m_tStartTime;
		time_t			m_tStageTime;
#ifdef WIN32
		HANDLE	 		m_hProcessID;
#else
		pid_t			m_hProcessID;
#endif
		

		void			SetProgressLabel(const char* szProgressLabel);
		
	public:
						PostJob(const char* szNZBFilename, const char* szDestDir, const char* szParFilename, 
							const char* szInfoName, bool bParCheck);
						~PostJob();
		const char*		GetNZBFilename() { return m_szNZBFilename; }
		const char*		GetDestDir() { return m_szDestDir; }
		const char*		GetParFilename() { return m_szParFilename; }
		const char*		GetInfoName() { return m_szInfoName; }
		EPostJobStage	GetStage() { return m_eStage; }
		const char*		GetProgressLabel() { return m_szProgressLabel; }
		int				GetFileProgress() { return m_iFileProgress; }
		int				GetStageProgress() { return m_iStageProgress; }
		time_t			GetStartTime() { return m_tStartTime; }
		time_t			GetStageTime() { return m_tStageTime; }

		friend class PrePostProcessor;
	};

	typedef std::deque<PostJob*> PostQueue;

private:
	typedef std::deque<char*>		FileList;

	class QueueCoordinatorObserver: public Observer
	{
	public:
		PrePostProcessor* owner;
		virtual void	Update(Subject* Caller, void* Aspect) { owner->QueueCoordinatorUpdate(Caller, Aspect); }
	};

#ifndef DISABLE_PARCHECK
	class ParCheckerObserver: public Observer
	{
	public:
		PrePostProcessor* owner;
		virtual void	Update(Subject* Caller, void* Aspect) { owner->ParCheckerUpdate(Caller, Aspect); }
	};

	class PostParChecker: public ParChecker
	{
	private:
		PrePostProcessor* m_Owner;
	protected:
		virtual bool	RequestMorePars(int iBlockNeeded, int* pBlockFound);
		virtual void	UpdateProgress();

		friend class PrePostProcessor;
	};

	struct BlockInfo
	{
		FileInfo*		m_pFileInfo;
		int				m_iBlockCount;
	};

	typedef std::deque<BlockInfo*> 	Blocks;
#endif
	
private:
	QueueCoordinatorObserver	m_QueueCoordinatorObserver;
	bool				m_bHasMoreJobs;
	bool				m_bPostScript;

	void				PausePars(DownloadQueue* pDownloadQueue, const char* szNZBFilename);
	void				CheckIncomingNZBs();
	bool				WasLastInCollection(DownloadQueue* pDownloadQueue, FileInfo* pFileInfo, bool bIgnorePaused);
	bool				IsNZBFileCompleted(DownloadQueue* pDownloadQueue, const char* szNZBFilename, bool bIgnoreFirstInPostQueue, bool bIgnorePaused);
	bool				CheckScript(FileInfo* pFileInfo);
	bool				JobExists(PostQueue* pPostQueue, const char* szNZBFilename);
	void				ClearCompletedJobs(const char* szNZBFilename);
	void				CheckPostQueue();
	void				JobCompleted(PostJob* pPostJob);
	void				StartScriptJob(PostJob* pPostJob);
	void				CheckScriptFinished(PostJob* pPostJob);

	Mutex			 	m_mutexQueue;
	PostQueue			m_PostQueue;
	PostQueue			m_CompletedJobs;

#ifndef DISABLE_PARCHECK
	PostParChecker		m_ParChecker;
	ParCheckerObserver	m_ParCheckerObserver;

	void				ParCheckerUpdate(Subject* Caller, void* Aspect);
	bool				CheckPars(DownloadQueue* pDownloadQueue, FileInfo* pFileInfo);
	bool				AddPar(FileInfo* pFileInfo, bool bDeleted);
	bool				SameParCollection(const char* szFilename1, const char* szFilename2);
	bool				FindMainPars(const char* szPath, FileList* pFileList);
	void				ParCleanupQueue(const char* szNZBFilename);
	bool				HasFailedParJobs(const char* szNZBFilename);
	bool				ParJobExists(PostQueue* pPostQueue, const char* szParFilename);
	bool				ParseParFilename(const char* szParFilename, int* iBaseNameLen, int* iBlocks);
	bool				RequestMorePars(const char* szNZBFilename, const char* szParFilename, int iBlockNeeded, int* pBlockFound);
	void				FindPars(DownloadQueue* pDownloadQueue, const char* szNZBFilename, const char* szParFilename, 
							Blocks* pBlocks, bool bStrictParName, bool bExactParName, int* pBlockFound);
	void				UpdateParProgress();
	void				StartParJob(PostJob* pPostJob);
#endif
	
public:
						PrePostProcessor();
	virtual				~PrePostProcessor();
	virtual void		Run();
	virtual void		Stop();
	void				QueueCoordinatorUpdate(Subject* Caller, void* Aspect);
	bool				HasMoreJobs() { return m_bHasMoreJobs; }
	PostQueue*			LockPostQueue();
	void				UnlockPostQueue();
};

#endif
