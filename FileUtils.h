//! @author prozac@rottenboy.com

#ifndef _FILEUTILS_H
#define _FILEUTILS_H

#include <unistd.h>
#include <ctype.h>
#include <sys/time.h>
#include <sys/file.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <stdio.h>
#include <signal.h>

#include "main.h"
#include <vector>
#include <map>
using std::vector;
using std::map;

class CFile {
public:
	CFile();
	CFile(const CString& sLongName);
	CFile(int iFD, const CString& sLongName);
	virtual ~CFile();

	enum EOptions {
		F_Read		= O_RDONLY,
		F_Write		= O_WRONLY,
		F_Create	= O_CREAT,
		F_Truncate	= O_TRUNC
	};

	enum EFileTypes {
		FT_REGULAR,
		FT_DIRECTORY,
		FT_CHARACTER,
		FT_BLOCK,
		FT_FIFO,
		FT_LINK,
		FT_SOCK
	};

	void SetFileName(const CString& sLongName);
	static bool IsReg(const CString& sLongName, bool bUseLstat = false);
	static bool IsDir(const CString& sLongName, bool bUseLstat = false);
	static bool IsChr(const CString& sLongName, bool bUseLstat = false);
	static bool IsBlk(const CString& sLongName, bool bUseLstat = false);
	static bool IsFifo(const CString& sLongName, bool bUseLstat = false);
	static bool IsLnk(const CString& sLongName, bool bUseLstat = true);
	static bool IsSock(const CString& sLongName, bool bUseLstat = false);

	bool IsReg(bool bUseLstat = false) const;
	bool IsDir(bool bUseLstat = false) const;
	bool IsChr(bool bUseLstat = false) const;
	bool IsBlk(bool bUseLstat = false) const;
	bool IsFifo(bool bUseLstat = false) const;
	bool IsLnk(bool bUseLstat = true) const;
	bool IsSock(bool bUseLstat = false) const;

	bool access(int mode);

	// for gettin file types, using fstat instead
	static bool FType(const CString sFileName, EFileTypes eType, bool bUseLstat = false);

	enum EFileAttr {
		FA_Name,
		FA_Size,
		FA_ATime,
		FA_MTime,
		FA_CTime,
		FA_UID
	};

	//
	// Functions to retrieve file information
	//
	bool Exists() const;
	unsigned long long GetSize() const;
	unsigned int GetATime() const;
	unsigned int GetMTime() const;
	unsigned int GetCTime() const;
	int GetUID() const;
	int GetGID() const;
	static bool Exists(const CString& sFile);

	static unsigned long long GetSize(const CString& sFile);
	static unsigned int GetATime(const CString& sFile);
	static unsigned int GetMTime(const CString& sFile);
	static unsigned int GetCTime(const CString& sFile);
	static int GetUID(const CString& sFile);
	static int GetGID(const CString& sFile);
	static int GetInfo(const CString& sFile, struct stat& st);

	//
	// Functions to manipulate the file on the filesystem
	//
	int Delete();
	int Move(const CString& sNewFileName, bool bOverwrite = false);
	int Copy(const CString& sNewFileName, bool bOverwrite = false);

	static bool Delete(const CString& sFileName);
	static bool Move(const CString& sOldFileName, const CString& sNewFileName, bool bOverwrite = false);
	static bool Copy(const CString& sOldFileName, const CString& sNewFileName, bool bOverwrite = false);
	bool Chmod(mode_t mode);
	static bool Chmod(const CString& sFile, mode_t mode);
	bool Seek(unsigned long uPos);
	bool Truncate();
	bool Open(const CString& sFileName, int iFlags, mode_t iMode = 0644);
	bool Open(int iFlags, mode_t iMode = 0644);
	int Read(char *pszBuffer, int iBytes);
	bool ReadLine(CString & sData, const CString & sDelimiter = "\n");
	int Write(const char *pszBuffer, u_int iBytes);
	int Write(const CString & sData);
	void Close();
	void ClearBuffer();

	bool IsOpen() const;
	CString GetLongName() const;
	CString GetShortName() const;
	CString GetDir() const;
	void SetFD(int iFD);

private:
	CString	m_sBuffer;
	int		m_iFD;

protected:
	CString	m_sLongName;	//!< Absolute filename (m_sPath + "/" + m_sShortName)
	CString	m_sShortName;	//!< Filename alone, without path
	bool	m_bClose;
};

class CDir : public vector<CFile*> {
public:

	CDir(const CString& sDir) {
		m_bDesc = false;
		m_eSortAttr = CFile::FA_Name;
		Fill(sDir);
	}

	CDir() {
		m_bDesc = false;
		m_eSortAttr = CFile::FA_Name;
	}

	virtual ~CDir() {
		CleanUp();
	}

	virtual void CleanUp() {
		for (unsigned int a = 0; a < size(); a++) {
			delete (*this)[a];
		}

		clear();
	}

	int Fill(const CString& sDir) {
		return FillByWildcard(sDir, "*");
	}

	/*void Sort(CFile::EFileAttr eSortAttr, bool bDesc = false) {
		m_eSortAttr = eSortAttr;
		m_bDesc = bDesc;
		sort(begin(), end(), TPtrCmp<CFile>);
	}*/

	static bool Exists(const CString& sDir) {
		CFile cFile(sDir);
		return (cFile.Exists()) && (cFile.IsDir());
	}

/*	static bool Create(const CString& sDir, mode_t mode = 0755) {
		VCCString vSubDirs = sDir.split("[/\\\\]+");
		CCString sCurDir;

		for (unsigned int a = 0; a < vSubDirs.size(); a++) {
			sCurDir += vSubDirs[a] + "/";
			if ((!CDir::Exists(sCurDir)) && (mkdir(sCurDir.c_str(), mode) != 0)) {
				return false;
			}
		}

		return true;
	}

	int FillByRegex(const CCString& sDir, const CCString& sRegex, const CCString& sFlags = "") {
		CleanUp();
		DIR* dir = opendir((sDir.empty()) ? "." : sDir.c_str());

		if (!dir) {
			return 0;
		}

		struct dirent * de;

		while ((de = readdir(dir)) != 0) {
			if ((strcmp(de->d_name, ".") == 0) || (strcmp(de->d_name, "..") == 0)) {
				continue;
			}
			if ((!sRegex.empty()) && (!CCString::search(de->d_name, sRegex, sFlags))) {
				continue;
			}

			CFile *file = new CFile(sDir, de->d_name, this);
			push_back(file);
		}

		closedir(dir);
		return size();
	}*/

	int FillByWildcard(const CString& sDir, const CString& sWildcard) {
		CleanUp();
		DIR* dir = opendir((sDir.empty()) ? "." : sDir.c_str());

		if (!dir) {
			return 0;
		}

		struct dirent * de;

		while ((de = readdir(dir)) != 0) {
			if ((strcmp(de->d_name, ".") == 0) || (strcmp(de->d_name, "..") == 0)) {
				continue;
			}
			if ((!sWildcard.empty()) && (!CString(de->d_name).WildCmp(sWildcard))) {
				continue;
			}

			CFile *file = new CFile(sDir + "/" + de->d_name/*, this*/);	// @todo need to pass pointer to 'this' if we want to do Sort()
			push_back(file);
		}

		closedir(dir);
		return size();
	}

	static unsigned int Chmod(mode_t mode, const CString& sWildcard, const CString& sDir = ".") {
		CDir cDir;
		cDir.FillByWildcard(sDir, sWildcard);
		return cDir.Chmod(mode);
	}

	unsigned int Chmod(mode_t mode) {
		unsigned int uRet = 0;
		for (unsigned int a = 0; a < size(); a++) {
			if ((*this)[a]->Chmod(mode)) {
				uRet++;
			}
		}

		return uRet;
	}

	static unsigned int Delete(mode_t mode, const CString& sWildcard, const CString& sDir = ".") {
		CDir cDir;
		cDir.FillByWildcard(sDir, sWildcard);
		return cDir.Delete();
	}

	unsigned int Delete() {
		unsigned int uRet = 0;
		for (unsigned int a = 0; a < size(); a++) {
			if ((*this)[a]->Delete()) {
				uRet++;
			}
		}

		return uRet;
	}

	CFile::EFileAttr GetSortAttr() { return m_eSortAttr; }
	bool IsDescending() { return m_bDesc; }

/*	static bool MkDir(const CString & sPath, mode_t iMode, bool bBuildParents = false, bool bApplyModToParents = false) {
		if (sPath.empty()) {
			WARN("empty path!");
			return false;
		}

		if (!bBuildParents) {	// only building target
			mode_t uMask = umask(0000);
			int iRet = mkdir(sPath.c_str(), iMode);
			umask(uMask);

			if (iRet != 0) {
				return false;
			}
		}


		VCCString vPath = sPath.TrimRight_n("/").split("/+");

		if (vPath.empty()) {
			return false;
		}

		if (sPath[0] == '/');
			vPath[0] = "/" + vPath[0];

		CCString sCurDir = GetCWD();

		mode_t uMask = 0000;
		if (bApplyModToParents) {
			uMask = umask(0000);
		}

		for (unsigned int a = 0; a < (vPath.size() - 1); a++) {
			if ((mkdir(vPath[a].c_str(), iMode) != 0) && (errno != EEXIST)) {
				if (bApplyModToParents) {
					umask(uMask);
				}

				return false;
			}

			if (chdir(vPath[a].c_str()) != 0) {
				chdir(sCurDir.c_str());
				if (bApplyModToParents) {
					umask(uMask);
				}

				return false;
			}
		}

		if (!bApplyModToParents) {
			uMask = umask(0000);
		}

		int iRet = mkdir(vPath[vPath.size() - 1].c_str(), iMode);
		umask(uMask);
		chdir(sCurDir.c_str());
		return (iRet == 0);
	}*/

	static CString GetCWD() {
		CString sRet;
		char * pszCurDir = getcwd(NULL, 0);
		if (pszCurDir) {
			sRet = pszCurDir;
			free(pszCurDir);
		}

		return sRet;
	}

private:
protected:
	CFile::EFileAttr	m_eSortAttr;
	bool				m_bDesc;
};

//! @author imaginos@imaginos.net
class CExecSock : public Csock {
public:
	CExecSock() : Csock() {
		m_iPid = -1;
	}

	CExecSock(const CString& sExec) : Csock() {
		Execute( sExec );
	}

	int Execute( const CString & sExec ) {
		int iReadFD, iWriteFD;
		m_iPid = popen2(iReadFD, iWriteFD, sExec);
		ConnectFD(iReadFD, iWriteFD, "0.0.0.0:0");
		return( m_iPid );
	}
	void Kill( int iSignal )
	{
		kill( m_iPid, iSignal );
		Close();
	}
	virtual ~CExecSock() {
		close2(m_iPid, GetRSock(), GetWSock());
		SetRSock( -1 );
		SetWSock( -1 );
	}

	int popen2(int & iReadFD, int & iWriteFD, const CString & sCommand);
	void close2(int iPid, int iReadFD, int iWriteFD);

private:
	int			m_iPid;
};
#endif // !_FILEUTILS_H
