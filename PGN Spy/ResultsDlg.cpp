// MIT License
// 
// Copyright(c) 2016 Michael J. Gleason
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files(the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and / or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions :
// 
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

// ResultsDlg.cpp : implementation file
//

#include "stdafx.h"
#include "PGN Spy.h"
#include "ResultsDlg.h"
#include "afxdialogex.h"


// CResultsDlg dialog

IMPLEMENT_DYNAMIC(CResultsDlg, CDialogEx)

CResultsDlg::CResultsDlg(CWnd* pParent /*=NULL*/)
   : CDialogEx(IDD_RESULTS, pParent)
   , m_sResults(_T(""))
{

}

CResultsDlg::~CResultsDlg()
{
}

void CResultsDlg::DoDataExchange(CDataExchange* pDX)
{
   CDialogEx::DoDataExchange(pDX);
   DDX_Text(pDX, IDC_RESULTS, m_sResults);
}


BEGIN_MESSAGE_MAP(CResultsDlg, CDialogEx)
   ON_BN_CLICKED(IDC_ABOUT, &CResultsDlg::OnBnClickedAbout)
   ON_BN_CLICKED(IDC_SAVEDATA, &CResultsDlg::OnBnClickedSavedata)
END_MESSAGE_MAP()

BOOL CResultsDlg::OnInitDialog()
{
   CDialog::OnInitDialog();

   CalculateStats();

   UpdateData(FALSE);

   return TRUE;  // return TRUE  unless you set the focus to a control
}

// CResultsDlg message handlers


void CResultsDlg::OnBnClickedAbout()
{
   CString sMessage;
   sMessage = "T1/T2/T3/etc: These stats display information about how often a player's moves matched the top\n"
              "one, two, three, etc., engine moves.  A high number could be an indicator of possible engine use.\n"
              "\n"
              "Blunder frequency: This indicates how often a player's moves were significantly worse than the\n"
              "top engine move.  A low number could be an indicator of possible engine use.\n"
              "\n"
              "Centipawn loss (average): This indicates, on average, how much worse a player's moves were when\n"
              "compared to the top engine move.  A low number could be an indicator of possible engine use.\n"
              "\n"
              "Centipawn loss (std deviation): This is the standard deviation of the centipawn loss.  A very low number\n"
              "could be an indicator of possible engine use, indicating that a player consistantly chooses moves of similar\n"
              "strength - unlikely for a human.  On the other hand, a very high number could indicate either a weak player,\n"
              "or a player who cheats in some positions and intentionally blunders at other times in an attempt to evade detection.\n"
              "\n"
              "These statistics MUST NOT be taken as evidence of cheating on their own, without proper statistical analysis,\n"
              "comparison to appropriate benchmarks, and consideration of other evidence.\n"
              "\n"
              "Binomial confidence intervals can be calculated at :"
              "\nhttp://statpages.info/confint.html#Binomial";
   MessageBox(sMessage, "PGN Spy", MB_ICONINFORMATION);
}


void CResultsDlg::OnBnClickedSavedata()
{
   CFileDialog vFileDialog(FALSE, _T("tab"), _T("*.tab"), OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, _T("Tab-delimited files (*.tab)|*.tab|All files (*.*)|*.*||"), this);
   if (vFileDialog.DoModal() != IDOK)
      return;
   CString sFilePath = vFileDialog.GetPathName();
   CString sReport, sLine, sText;
   sReport = "Event\tDate\tWhite\tBlack\tResult\tTime Control\tMove #\tMove Played\tDepth Searched\tT-number";
   for (int i = 0; i < m_vSettings.m_iNumVariations; i++)
   {
      sText.Format("\tT%i move eval", i + 1);
      sReport += sText;
   }
   sReport += "\tNon-top move eval";
   for (int iGame = 0; iGame < m_avGames.GetSize(); iGame++)
   {
      CGame *pGame = &m_avGames[iGame];
      for (int iPosition = 0; iPosition < pGame->m_avPositions.GetSize(); iPosition++)
      {
         //first get general game data
         CPosition *pPosition = &pGame->m_avPositions[iPosition];
         sLine = pGame->m_sEvent + "\t" + pGame->m_sDate + "\t" + pGame->m_sWhite + "\t" + pGame->m_sBlack + "\t" + pGame->m_sResult + "\t" + pGame->m_sTimeControl + "\t";

         //got all game data, now get move data
         //move number and coordinates
         sText.Format("%i\t", iPosition + m_vSettings.m_iBookDepth + 1);
         sLine += sText + pPosition->m_avTopMoves[pPosition->m_iMovePlayed].m_sMove + "\t";
         //max depth searched
         int iMaxDepth = 0;
         for (int i = 0; i < pPosition->m_avTopMoves.GetSize(); i++)
            iMaxDepth = max(iMaxDepth, pPosition->m_avTopMoves[i].m_iDepth);
         sText.Format("%i\t", iMaxDepth);
         sLine += sText;
         //T-number of move
         sText.Format("%i\t", pPosition->m_iMovePlayed + 1);
         sLine += sText;
         //evaluation of variants
         int iMove = 0;
         for (; iMove < pPosition->m_avTopMoves.GetSize(); iMove++)
         {
            sText.Format("%i\t", pPosition->m_avTopMoves[iMove].m_iScore);
            sLine += sText;
         }
         for (; iMove < m_vSettings.m_iNumVariations + 1; iMove++)
            sLine += "N/A\t";
         sReport += "\r\n" + sLine;
      }
   }

   CFile vFile;
   if (!vFile.Open(sFilePath, CFile::modeCreate | CFile::modeWrite))
   {
      MessageBox("Failed to create output file.", "PGN Spy", MB_ICONEXCLAMATION);
      return;
   }

   vFile.Write(sReport.GetBuffer(), sReport.GetLength());
   sReport.ReleaseBuffer();
   vFile.Close();
   MessageBox("File saved.", "PGN Spy", MB_ICONINFORMATION);

   ShellExecute(NULL, "open", sFilePath, NULL, NULL, SW_MAXIMIZE);
}

void CResultsDlg::CalculateStats()
{
   m_vUndecidedPositions.Initialize(m_vSettings);
   m_vLosingPositions.Initialize(m_vSettings);

   //calculate results
   for (int iGame = 0; iGame < m_avGames.GetSize(); iGame++)
   {
      for (int iPosition = 0; iPosition < m_avGames[iGame].m_avPositions.GetSize(); iPosition++)
      {
         CPosition *pPosition = &m_avGames[iGame].m_avPositions[iPosition];
         if (pPosition->IsEqualPosition(m_vSettings.m_iEqualPositionThreshold))
            m_vUndecidedPositions.AddPosition(*pPosition, m_vSettings);
         if (pPosition->IsLosingPosition(m_vSettings.m_iEqualPositionThreshold, m_vSettings.m_iLosingThreshold))
            m_vLosingPositions.AddPosition(*pPosition, m_vSettings);
      }
   }

   m_vUndecidedPositions.FinaliseStats();
   m_vLosingPositions.FinaliseStats();

   //Now dump results to text
   CString sLine;
   m_sResults = "UNDECIDED POSITIONS\r\n";
   sLine.Format("Positions: %i", m_vUndecidedPositions.m_iNumPositions);
   m_sResults += sLine + "\r\n";
   if (m_vUndecidedPositions.m_iNumPositions > 0)
   {
      //T-values
      for (int i = 0; i < m_vSettings.m_iNumVariations; i++)
      {
         if (m_vUndecidedPositions.m_aiTMoves[i] == 0)
            sLine.Format("T%i: 0/0", i);
         else
         {
            double dFrac = ((double)m_vUndecidedPositions.m_aiTValues[i] / (double)m_vUndecidedPositions.m_aiTMoves[i]);
            double dStdError = sqrt(dFrac * (1 - dFrac) / m_vUndecidedPositions.m_aiTMoves[i]) * 100;
            sLine.Format("T%i: %i/%i; %.2f%% (std error %.2f)", i + 1, m_vUndecidedPositions.m_aiTValues[i], m_vUndecidedPositions.m_aiTMoves[i], dFrac*100.0, dStdError);
         }
         m_sResults += sLine + "\r\n";
      }
      //blunders
      {
         double dFrac = ((double)m_vUndecidedPositions.m_iBlunders / (double)m_vUndecidedPositions.m_iNumPositions);
         double dStdError = sqrt(dFrac * (1 - dFrac) / m_vUndecidedPositions.m_iNumPositions) * 100;
         sLine.Format("Blunders: %i/%i; %.2f%% (std error %.2f)", m_vUndecidedPositions.m_iBlunders, m_vUndecidedPositions.m_iNumPositions, dFrac*100.0, dStdError);
         m_sResults += sLine + "\r\n";
      }

      sLine.Format("Avg centipawn loss: %.2f (std deviation %.2f)", m_vUndecidedPositions.m_dAvgCentipawnLoss, m_vUndecidedPositions.m_dCentipawnLossStdDeviation);
      m_sResults += sLine + "\r\n";
   }
   m_sResults += "\r\n";

   sLine = "LOSING POSITIONS";
   m_sResults += sLine + "\r\n";
   sLine.Format("Positions: %i", m_vLosingPositions.m_iNumPositions);
   m_sResults += sLine + "\r\n";
   if (m_vLosingPositions.m_iNumPositions > 0)
   {
      //T-values
      for (int i = 0; i < m_vSettings.m_iNumVariations; i++)
      {
         if (m_vLosingPositions.m_aiTMoves[i] == 0)
            sLine.Format("T%i: 0/0", i);
         else
         {
            double dFrac = ((double)m_vLosingPositions.m_aiTValues[i] / (double)m_vLosingPositions.m_aiTMoves[i]);
            double dStdError = sqrt(dFrac * (1 - dFrac) / m_vLosingPositions.m_aiTMoves[i]) * 100;
            sLine.Format("T%i: %i/%i; %.2f%% (std error %.2f)", i + 1, m_vLosingPositions.m_aiTValues[i], m_vLosingPositions.m_aiTMoves[i], dFrac*100.0, dStdError);
         }
         m_sResults += sLine + "\r\n";
      }
      //blunders
      {
         double dFrac = ((double)m_vLosingPositions.m_iBlunders / (double)m_vLosingPositions.m_iNumPositions);
         double dStdError = sqrt(dFrac * (1 - dFrac) / m_vLosingPositions.m_iNumPositions) * 100;
         sLine.Format("Blunders: %i/%i; %.2f%% (std error %.2f)", m_vLosingPositions.m_iBlunders, m_vLosingPositions.m_iNumPositions, dFrac*100.0, dStdError);
         m_sResults += sLine + "\r\n";
      }

      sLine.Format("Avg centipawn loss: %.2f (std deviation %.2f)", m_vLosingPositions.m_dAvgCentipawnLoss, m_vLosingPositions.m_dCentipawnLossStdDeviation);
      m_sResults += sLine + "\r\n";
   }
}