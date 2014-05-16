#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "cqtmfc_famitracker.h"

#include "Source/FamiTracker.h"
#include "Source/FamiTrackerDoc.h"
#include "Source/FamiTrackerView.h"
#include "Source/PatternEditor.h"
#include "Source/MainFrm.h"
#include "Source/SoundGen.h"
#include "Source/SampleWindow.h"
#include "Source/Settings.h"

#include "playlisteditordialog.h"
#include "aboutdialog.h"

#include <QFileInfo>
#include <QCompleter>
#include <QUrl>
#include <QDateTime>

IMPLEMENT_DYNAMIC(CWndMFC,CDialog)

BEGIN_MESSAGE_MAP(CWndMFC,CDialog)
   ON_NOTIFY(UDN_DELTAPOS, IDC_SPIN_TIME, OnDeltaposSpinTime)
   ON_NOTIFY(UDN_DELTAPOS, IDC_SPIN_LOOP, OnDeltaposSpinLoop)
   ON_EN_CHANGE(IDC_TIMES,OnEnChangeTimes)
   ON_EN_CHANGE(IDC_SECONDS,OnEnChangeSeconds)
END_MESSAGE_MAP()

BOOL CWndMFC::OnInitDialog()
{
   QSettings settings(QSettings::IniFormat, QSettings::UserScope, "CSPSoftware", "FamiPlayer");
   CString str;
   int Minutes, Seconds;
   int Time = settings.value("PlayTime",180).toInt();
   int Loops = settings.value("PlayLoops",1).toInt();
   if ( Time == 0 )
   {
      Time = 10;
   }
   if ( Loops == 0 )
   {
      Loops = 1;
   }

	if (Time < 1)
		Time = 1;
//	if (Time > MAX_PLAY_TIME)
//		Time = MAX_PLAY_TIME;

	Seconds = Time % 60;
	Minutes = Time / 60;

	str.Format(_T("%02i:%02i"), Minutes, Seconds);
	SetDlgItemText(IDC_SECONDS, str);
   
   SetDlgItemInt(IDC_TIMES, Loops);   
}

int CWndMFC::GetFrameLoopCount()
{
	int Frames = GetDlgItemInt(IDC_TIMES);

	if (Frames < 1)
		Frames = 1;
//	if (Frames > MAX_LOOP_TIMES)
//		Frames = MAX_LOOP_TIMES;

	return Frames;
}

int CWndMFC::ConvertTime(LPCTSTR str)
{
   int Minutes, Seconds, Time;

   _stscanf(str, _T("%u:%u"), &Minutes, &Seconds);
	Time = (Minutes * 60) + (Seconds % 60);

	if (Time < 1)
		Time = 1;
//	if (Time > MAX_PLAY_TIME)
//		Time = MAX_PLAY_TIME;

	return Time;
}

int CWndMFC::GetTimeLimit()
{
	TCHAR str[256];

	GetDlgItemText(IDC_SECONDS, str, 256);
   return ConvertTime(str);
}

void CWndMFC::OnEnChangeTimes()
{
   QSettings settings(QSettings::IniFormat, QSettings::UserScope, "CSPSoftware", "FamiPlayer");
   int Loops = GetFrameLoopCount();

   settings.setValue("PlayLoops",Loops);
}

void CWndMFC::OnEnChangeSeconds()
{
   QSettings settings(QSettings::IniFormat, QSettings::UserScope, "CSPSoftware", "FamiPlayer");
   int Time = GetTimeLimit();

   settings.setValue("PlayTime",Time);
}

void CWndMFC::OnDeltaposSpinLoop(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMUPDOWN pNMUpDown = reinterpret_cast<LPNMUPDOWN>(pNMHDR);
	int Times = GetFrameLoopCount() - pNMUpDown->iDelta;

	if (Times < 1)
		Times = 1;
//	if (Times > MAX_LOOP_TIMES)
//		Times = MAX_LOOP_TIMES;

	SetDlgItemInt(IDC_TIMES, Times);
	*pResult = 0;
}

void CWndMFC::OnDeltaposSpinTime(NMHDR *pNMHDR, LRESULT *pResult)
{
   QSettings settings(QSettings::IniFormat, QSettings::UserScope, "CSPSoftware", "FamiPlayer");
	LPNMUPDOWN pNMUpDown = reinterpret_cast<LPNMUPDOWN>(pNMHDR);
	int Minutes, Seconds;
	int Time = GetTimeLimit() - pNMUpDown->iDelta;
	CString str;

	if (Time < 1)
		Time = 1;
//	if (Time > MAX_PLAY_TIME)
//		Time = MAX_PLAY_TIME;

	Seconds = Time % 60;
	Minutes = Time / 60;

	str.Format(_T("%02i:%02i"), Minutes, Seconds);
	SetDlgItemText(IDC_SECONDS, str);
   
   settings.setValue("PlayTime",Time);
	*pResult = 0;
}

MainWindow::MainWindow(QWidget *parent) :
   QMainWindow(parent),
   ui(new Ui::MainWindow)
{
   QSettings settings(QSettings::IniFormat, QSettings::UserScope, "CSPSoftware", "FamiPlayer");
   
   ui->setupUi(this);
   
   m_pTimer = new QTimer;
   m_pSettleTimer = new QTimer;
   
   // Initialize the app...
   backgroundifyFamiTracker("FamiPlayer");
   qtMfcInit(this);
   theApp.InitInstance();

   ui->paths->setDuplicatesEnabled(false);
      
   ui->indicators->layout()->addWidget(AfxGetMainWnd()->GetDescendantWindow(AFX_IDW_STATUS_BAR)->GetDlgItem(ID_INDICATOR_TIME)->toQWidget());

   m_bChangeSong = false;
   
   // Enable grabbing the slider while playing.
   ui->position->installEventFilter(this);
   
   m_bDraggingPosition = false;
   
   // Create play-time widget...from MFC stuff.
   m_pWndMFC = new CWndMFC();   
   m_pWndMFC->Create(0,AfxGetMainWnd());
   QRect qrect = QRect(0,0,132,21);
   m_pWndMFC->setFixedSize(qrect.width(),qrect.height());
//       EDITTEXT        IDC_SECONDS,53,37,44,12,ES_AUTOHSCROLL
   CRect r9(0,0,qrect.width()/2,qrect.height());
   CEdit* mfc9 = new CEdit(m_pWndMFC);
   mfc9->Create(ES_AUTOHSCROLL | WS_VISIBLE,r9,m_pWndMFC,IDC_SECONDS);
   mfc9->toQWidget()->setToolTip("Seconds to Play");
   m_pWndMFC->mfcToQtWidgetMap()->insert(IDC_SECONDS,mfc9);
//       CONTROL         "",IDC_SPIN_TIME,"msctls_updown32",UDS_ALIGNRIGHT | UDS_AUTOBUDDY | UDS_ARROWKEYS,93,36,11,14
   CSpinButtonCtrl* mfc10 = new CSpinButtonCtrl(m_pWndMFC);
   CRect r10(CPoint(0,0),CSize(16,14));
   mfc10->Create(UDS_ALIGNRIGHT | UDS_AUTOBUDDY | UDS_ARROWKEYS | WS_VISIBLE,r10,m_pWndMFC,IDC_SPIN_TIME);   
   mfc10->toQWidget()->setToolTip("Seconds to Play");
   m_pWndMFC->mfcToQtWidgetMap()->insert(IDC_SPIN_TIME,mfc10);
   ui->playTime->layout()->addWidget(m_pWndMFC->toQWidget());
//       EDITTEXT        IDC_TIMES,73,19,36,12,ES_AUTOHSCROLL
   CEdit* mfc6 = new CEdit(m_pWndMFC);
   CRect r6((qrect.width()/2)+1,0,qrect.width(),qrect.height());
   mfc6->Create(ES_AUTOHSCROLL | WS_VISIBLE,r6,m_pWndMFC,IDC_TIMES);
   mfc6->toQWidget()->setToolTip("Loops to Play");
   m_pWndMFC->mfcToQtWidgetMap()->insert(IDC_TIMES,mfc6);
//       CONTROL         "",IDC_SPIN_LOOP,"msctls_updown32",UDS_ALIGNRIGHT | UDS_AUTOBUDDY | UDS_ARROWKEYS,105,17,11,17
   CSpinButtonCtrl* mfc7 = new CSpinButtonCtrl(m_pWndMFC);
   CRect r7(CPoint((qrect.width()/2)+1,0),CSize(16,14));
   mfc7->Create(UDS_ALIGNRIGHT | UDS_AUTOBUDDY | UDS_ARROWKEYS | WS_VISIBLE,r7,m_pWndMFC,IDC_SPIN_LOOP);
   mfc7->toQWidget()->setToolTip("Loops to Play");
   m_pWndMFC->mfcToQtWidgetMap()->insert(IDC_SPIN_LOOP,mfc7);
   
   m_pWndMFC->OnInitDialog();
   
   ui->sampleWindow->installEventFilter(this);
   
   ui->current->completer()->setCompletionMode(QCompleter::PopupCompletion);
   QObject::connect(ui->current->lineEdit(),SIGNAL(returnPressed()),this,SLOT(current_returnPressed()));
   ui->current->setInsertPolicy(QComboBox::NoInsert);
   
   ui->visuals->setChecked(settings.value("Visuals",true).toBool());
   on_visuals_toggled(settings.value("Visuals",true).toBool());
   
   restoreGeometry(settings.value("WindowGeometry").toByteArray());
   restoreState(settings.value("WindowState").toByteArray());
   
   if ( settings.value("PlaylistFile").toString().isEmpty() )
   {
      updateUiFromINI();
   }
   else
   {
      updateUiFromPlaylist();               
   }
   
   ui->timeLimit->setChecked(settings.value("TimeLimit",true).toBool());
   on_timeLimit_toggled(settings.value("TimeLimit",true).toBool());
   
   m_iCurrentShuffleIndex = 0;
   ui->shuffle->setChecked(settings.value("Shuffle",false).toBool());
   on_shuffle_toggled(settings.value("Shuffle",false).toBool());
   
   ui->repeat->setChecked(settings.value("Repeat",false).toBool());
   on_repeat_toggled(settings.value("Repeat",false).toBool());

   m_bPlaying = false;
   
   ui->playOnStart->setChecked(settings.value("PlayOnStart",false).toBool());
   if ( settings.value("PlayOnStart",false).toBool() )
   {
      on_playStop_clicked();
   }
   
   QObject::connect(m_pTimer,SIGNAL(timeout()),this,SLOT(onIdleSlot()));
   m_pTimer->start();
   
   QObject::connect(m_pSettleTimer,SIGNAL(timeout()),this,SLOT(settleTimer_timeout()));
   m_bCheck = true;
}

MainWindow::~MainWindow()
{   
   delete ui;
   delete m_pTimer;
   delete m_pSettleTimer;
}

bool MainWindow::eventFilter(QObject *object, QEvent *event)
{
   if ( object == ui->position && event->type() == QEvent::MouseButtonPress )
   {
      m_bDraggingPosition = true;
      return false;
   }
   else if ( object == ui->position && event->type() == QEvent::MouseButtonRelease )
   {
      m_bDraggingPosition = false;
      return false;
   }
   else if ( object == ui->sampleWindow && event->type() == QEvent::MouseButtonPress )
   {
      CMainFrame* pMainFrame = (CMainFrame*)AfxGetMainWnd();
      pMainFrame->GetSampleWindow()->SendMessage(WM_LBUTTONDOWN);      
   }
   else if ( object == ui->sampleWindow && event->type() == QEvent::Paint )
   {      
      CMainFrame* pMainFrame = (CMainFrame*)AfxGetMainWnd();
      CFamiTrackerDoc* pDoc = (CFamiTrackerDoc*)pMainFrame->GetActiveDocument();
      CFamiTrackerView* pView = (CFamiTrackerView*)pMainFrame->GetActiveView();
      
      QPainter p;
      QRect rect = pMainFrame->GetSampleWindow()->toQWidget()->rect().adjusted(2,2,-3,-3);
      QPixmap pixmap(rect.size());
      p.begin(&pixmap);
      pMainFrame->GetSampleWindow()->toQWidget()->render(&p,QPoint(),QRegion(3,3,rect.width(),rect.height()));
      p.end();
      p.begin(ui->sampleWindow);
      p.drawPixmap(0,0,pixmap.scaled(p.window().size(),Qt::IgnoreAspectRatio,Qt::FastTransformation));
      p.end();
      return true;
   }
   return false;
}

void MainWindow::settleTimer_timeout()
{
   m_bCheck = true;
   m_pSettleTimer->stop();
}

void MainWindow::onIdleSlot()
{
   CFamiTrackerApp* pApp = (CFamiTrackerApp*)AfxGetApp();   
   CMainFrame* pMainFrame = (CMainFrame*)pApp->m_pMainWnd;
   CFamiTrackerDoc* pDoc = (CFamiTrackerDoc*)pMainFrame->GetActiveDocument();
   CFamiTrackerView* pView = (CFamiTrackerView*)pMainFrame->GetActiveView();
   static int lastFrame = -1;
   
   ui->sampleWindow->update();

   if ( m_bCheck )
   {
      // Move to next song if playing and the current song has reached a stop.
      if ( m_bCheck && m_bPlaying && !m_bChangeSong )
      {
         // Check if time is at time limit and if so, advance to next song.
         int timeLimit = m_pWndMFC->GetTimeLimit();
         CString playTime;
         int totalPlayTime;
         int loopCount;
         
         pMainFrame->GetDescendantWindow(AFX_IDW_STATUS_BAR)->GetDlgItemText(ID_INDICATOR_TIME,playTime);
         totalPlayTime = m_pWndMFC->ConvertTime(playTime);
         loopCount = m_pWndMFC->GetDlgItemInt(IDC_TIMES);
         
         if ( ui->timeLimit->isChecked() )
         {
            if ( timeLimit == totalPlayTime )
            {
               // Force stop...
               m_bPlaying = false;
               pApp->OnCmdMsg(ID_TRACKER_TOGGLE_PLAY,0,0,0);
               m_bChangeSong = true;
               
               // Create a bit of a delay between songs.
               m_pTimer->start(500);
            }
            else
            {   
               if (m_iFramesPlayed > pDoc->ScanActualLength(pDoc->GetSelectedTrack(),m_pWndMFC->GetFrameLoopCount()))
               {
                  // Force stop...
                  m_bPlaying = false;
                  pApp->OnCmdMsg(ID_TRACKER_TOGGLE_PLAY,0,0,0);
                  m_bChangeSong = true;
                  
                  // Create a bit of a delay between songs.
                  m_pTimer->start(500);
               }
               if ( lastFrame != pView->GetPlayFrame() )
               {
                  lastFrame = pView->GetPlayFrame();
                  m_iFramesPlayed++;
               }
            }
         }
         if ( !pApp->GetSoundGenerator()->IsPlaying() )
         {
            m_bPlaying = false;
            m_bChangeSong = true;
            
            // Create a bit of a delay between songs.
            m_pTimer->start(500);
         }
      }
      // Wait until player starts playing before turning the above logic back on.
      else if ( m_bChangeSong )
      {
         on_playStop_clicked();
         if ( !ui->repeat->isChecked() )
         {
            on_next_clicked();
         }
         m_bChangeSong = false;
         ui->position->setValue((pView->GetPatternView()->GetPlayFrame()*pDoc->GetPatternLength())+pView->GetPatternView()->GetPlayRow());
         startSettleTimer();
         m_pTimer->start(0);
      }
   }
   if ( m_bDraggingPosition )
   {
      // FF/RW
      pView->PlayerCommand(CMD_JUMP_TO,(ui->position->value()/pDoc->GetPatternLength())-1);
      pView->GetPatternView()->JumpToRow(ui->position->value()%pDoc->GetPatternLength());
   }
   else
   {
      ui->frames->setText(QString::number(m_iFramesPlayed)+"/"+QString::number(pDoc->ScanActualLength(pDoc->GetSelectedTrack(),m_pWndMFC->GetFrameLoopCount())));
      ui->position->setValue((pView->GetPatternView()->GetPlayFrame()*pDoc->GetPatternLength())+pView->GetPatternView()->GetPlayRow());
   }
}

void MainWindow::startSettleTimer()
{
   m_pSettleTimer->start(800);
   m_bCheck = false;
}

void MainWindow::documentClosed()
{
   // TODO: Handle unsaved documents or other pre-close stuffs
   theApp.ExitInstance();

   exit(0);
}

void MainWindow::dragEnterEvent(QDragEnterEvent *event)
{
   if ( event->mimeData()->hasUrls() )
   {
      event->acceptProposedAction();
   }
}

void MainWindow::dragMoveEvent(QDragMoveEvent *event)
{
   if ( event->mimeData()->hasUrls() )
   {
      event->acceptProposedAction();
   }
}

void MainWindow::dropEvent(QDropEvent *event)
{
   QSettings settings(QSettings::IniFormat, QSettings::UserScope, "CSPSoftware", "FamiPlayer");
   QList<QUrl> fileUrls;
   QString     fileName;
   QFileInfo   fileInfo;
   QStringList extensions;

   if ( event->mimeData()->hasUrls() )
   {
      fileUrls = event->mimeData()->urls();

      foreach ( QUrl fileUrl, fileUrls )
      {
         fileName = fileUrl.toLocalFile();

         fileInfo.setFile(fileName);

         if ( !fileInfo.suffix().compare("fpl",Qt::CaseInsensitive) )
         {
            bool wasPlaying = m_bPlaying;
            if ( wasPlaying )
            {
               on_playStop_clicked();
            }
            settings.setValue("PlaylistFile",fileInfo.filePath());
            updateUiFromPlaylist();
            if ( wasPlaying )
            {
               on_playStop_clicked();
            }
            event->acceptProposedAction();
         }
      }
   }
}

void MainWindow::closeEvent(QCloseEvent *event)
{
   QSettings settings(QSettings::IniFormat, QSettings::UserScope, "CSPSoftware", "FamiPlayer");
   
   settings.setValue("WindowGeometry",saveGeometry());
   settings.setValue("WindowState",saveState());

   // CP: Force synchronization because we're terminating in OnClose and the settings object
   // can't synchronize to disk if we wait for that.
   settings.sync();
   
   AfxGetMainWnd()->OnClose();

   // Ignore the close event.  "MFC" will close the document which will trigger app closure.
   event->ignore();
}

void MainWindow::on_browse_clicked()
{
   QSettings settings(QSettings::IniFormat, QSettings::UserScope, "CSPSoftware", "FamiPlayer");
   QStringList folderList = settings.value("FolderList").toStringList();
   QString lastFolder = settings.value("LastFolder").toString();
   lastFolder = QFileDialog::getExistingDirectory(0,"Select a folder of FTMs...",lastFolder,0);
   if ( !lastFolder.isEmpty() )
   {
      ui->paths->addItem(lastFolder);
      settings.setValue("LastFolder",lastFolder);

      QDir dir(lastFolder);
      QFileInfoList fileInfos = dir.entryInfoList(QStringList("*.ftm"));
      if ( fileInfos.count() )
      {
         folderList.append(lastFolder);
         folderList.removeDuplicates();
         settings.setValue("FolderList",folderList);
         changeFolder(lastFolder);
         createShuffleLists();
      }
   }
}

void MainWindow::on_playStop_clicked()
{
   AfxGetApp()->OnCmdMsg(ID_TRACKER_TOGGLE_PLAY,0,0,0);
   m_bPlaying = !m_bPlaying;
   if ( m_bPlaying )
   {
      ui->playStop->setIcon(QIcon(":/resources/Pause-icon.png"));
   }
   else
   {
      ui->playStop->setIcon(QIcon(":/resources/stock_media-play.png"));
   }
}

void MainWindow::on_previous_clicked()
{
   CMainFrame* pMainFrame = (CMainFrame*)AfxGetMainWnd();
   CFamiTrackerView* pView = (CFamiTrackerView*)pMainFrame->GetActiveView();
   CFamiTrackerDoc* pDoc = (CFamiTrackerDoc*)pMainFrame->GetActiveDocument();
   
   if ( pDoc->GetSelectedTrack() > 0 )
   {
      ui->subtune->setCurrentIndex(ui->subtune->currentIndex()-1);
   }
   else
   {
      if ( ui->shuffle->isChecked() )
      {
         if ( m_iCurrentShuffleIndex > 0 )
         {
            m_iCurrentShuffleIndex--;
         }
         else
         {
            m_iCurrentShuffleIndex = m_shuffleListFolder.count()-1;
         }
         if ( m_iCurrentShuffleIndex < m_shuffleListFolder.count() )
         {
            ui->paths->setCurrentIndex(ui->paths->findText(m_shuffleListFolder.at(m_iCurrentShuffleIndex)));
            ui->current->setCurrentIndex(ui->current->findText(m_shuffleListSong.at(m_iCurrentShuffleIndex)));
         }
      }
      else
      {
         if ( ui->current->currentIndex() > 0 )
         {
            ui->current->setCurrentIndex(ui->current->currentIndex()-1);
         }
         else
         {
            if ( ui->paths->currentIndex() > 0 )
            {
               ui->paths->setCurrentIndex(ui->paths->currentIndex()-1);
            }
            else
            {
               ui->paths->setCurrentIndex(ui->paths->count()-1);
            }
            ui->current->setCurrentIndex(ui->current->count()-1);
         }
      }
      ui->subtune->setCurrentIndex(ui->subtune->count()-1);
   }
   
   m_iFramesPlayed = 0;     
}

void MainWindow::on_next_clicked()
{
   CMainFrame* pMainFrame = (CMainFrame*)AfxGetMainWnd();
   CFamiTrackerView* pView = (CFamiTrackerView*)pMainFrame->GetActiveView();
   CFamiTrackerDoc* pDoc = (CFamiTrackerDoc*)pMainFrame->GetActiveDocument();

   if ( pDoc->GetSelectedTrack() < pDoc->GetTrackCount()-1 )
   {
      ui->subtune->setCurrentIndex(ui->subtune->currentIndex()+1);
   }
   else
   {
      if ( ui->shuffle->isChecked() )
      {
         if ( m_iCurrentShuffleIndex < m_shuffleListFolder.count()-1 )
         {
            m_iCurrentShuffleIndex++;
         }
         else
         {
            m_iCurrentShuffleIndex = 0;
         }
         if ( m_iCurrentShuffleIndex < m_shuffleListFolder.count() )
         {
            ui->paths->setCurrentIndex(ui->paths->findText(m_shuffleListFolder.at(m_iCurrentShuffleIndex)));
            ui->current->setCurrentIndex(ui->current->findText(m_shuffleListSong.at(m_iCurrentShuffleIndex)));
         }
      }
      else
      {
         if ( ui->current->currentIndex() < ui->current->count()-1 )
         {
            ui->current->setCurrentIndex(ui->current->currentIndex()+1);
         }
         else
         {
            if ( ui->paths->currentIndex() < ui->paths->count()-1 )
            {
               ui->paths->setCurrentIndex(ui->paths->currentIndex()+1);
            }
            else
            {
               ui->paths->setCurrentIndex(0);
            }
            ui->current->setCurrentIndex(0);
         }
      }
      ui->subtune->setCurrentIndex(0);
   }
   
   m_iFramesPlayed = 0;     
}

void MainWindow::on_paths_currentIndexChanged(const QString &arg1)
{
   bool wasPlaying = m_bPlaying;
   
   if ( wasPlaying )
   {
      on_playStop_clicked();
   }
   
   changeFolder(arg1);
   
   m_iFramesPlayed = 0;     
   
   if ( wasPlaying )
   {
      on_playStop_clicked();
   }
}

void MainWindow::on_current_currentIndexChanged(const QString &arg1)
{
   bool wasPlaying = m_bPlaying;
   
   if ( wasPlaying )
   {
      on_playStop_clicked();
   }
   
   changeSong(arg1);
   
   m_iFramesPlayed = 0;     
   
   if ( wasPlaying )
   {
      on_playStop_clicked();
   }
}

void MainWindow::changeFolder(QString newFolderPath)
{
   QSettings settings(QSettings::IniFormat, QSettings::UserScope, "CSPSoftware", "FamiPlayer");
   
   settings.setValue("CurrentFolder",newFolderPath);

   QDir dir(ui->paths->currentText());
   QFileInfoList fileInfos = dir.entryInfoList(QStringList("*.ftm"));
   ui->current->clear();
   foreach ( QFileInfo fileInfo, fileInfos )
   {
      ui->current->addItem(fileInfo.fileName(),fileInfo.filePath());
   }
}

void MainWindow::changeSong(QString newSongPath)
{
   if ( ui->current->currentIndex() != -1 )
   {
      QSettings settings(QSettings::IniFormat, QSettings::UserScope, "CSPSoftware", "FamiPlayer");
      QFileInfo fileInfo(ui->current->itemData(ui->current->currentIndex()).toString());
      CMainFrame* pMainFrame = (CMainFrame*)AfxGetMainWnd();
      CFamiTrackerDoc* pDoc = (CFamiTrackerDoc*)pMainFrame->GetActiveDocument();
      
      loadFile(fileInfo.filePath());
      ui->current->setToolTip(fileInfo.filePath());
      updateSubtuneText();
   }
}

void MainWindow::updateSubtuneText()
{   
   CMainFrame* pMainFrame = (CMainFrame*)AfxGetMainWnd();
   CFamiTrackerDoc* pDoc = (CFamiTrackerDoc*)pMainFrame->GetActiveDocument();
   QString subtune;
   int song;

   ui->subtune->clear();
   for ( song = 0; song < pDoc->GetTrackCount(); song++ )
   {
      subtune.sprintf("#%d %s",song+1,pDoc->GetTrackTitle(song));
      ui->subtune->addItem(subtune);
   }
}

void MainWindow::loadFile(QString file)
{   
   QSettings settings(QSettings::IniFormat, QSettings::UserScope, "CSPSoftware", "FamiPlayer");
   CMainFrame* pMainFrame = (CMainFrame*)AfxGetMainWnd();
   CFamiTrackerDoc* pDoc = (CFamiTrackerDoc*)pMainFrame->GetActiveDocument();
   startSettleTimer();
   openFile(file);
   settings.setValue("CurrentFile",file);
   
   m_iFramesPlayed = 0;
   
   ui->position->setRange(0,(pDoc->GetFrameCount()*pDoc->GetPatternLength()));
   ui->position->setPageStep(pDoc->GetPatternLength());
}

void MainWindow::on_subtune_currentIndexChanged(int index)
{
   QSettings settings(QSettings::IniFormat, QSettings::UserScope, "CSPSoftware", "FamiPlayer");
   
   if ( index != -1 )
   {
      CMainFrame* pMainFrame = (CMainFrame*)AfxGetMainWnd();
      CFamiTrackerView* pView = (CFamiTrackerView*)pMainFrame->GetActiveView();
      CFamiTrackerDoc* pDoc = (CFamiTrackerDoc*)pMainFrame->GetActiveDocument();   
      int current = pDoc->GetSelectedTrack();
      bool wasPlaying = m_bPlaying;
      
      if ( current < index )
      {
         if ( wasPlaying )
         {
            on_playStop_clicked();
         }
         while ( current != index )
         {
            AfxGetMainWnd()->OnCmdMsg(ID_NEXT_SONG,0,0,0);
            current++;
         }
         if ( wasPlaying )
         {
            on_playStop_clicked();
         }
      }      
      else
      {
         if ( wasPlaying )
         {
            on_playStop_clicked();
         }
         while ( current != index )
         {
            AfxGetMainWnd()->OnCmdMsg(ID_PREV_SONG,0,0,0);
            current--;
         }
         if ( wasPlaying )
         {
            on_playStop_clicked();
         }
      }
      
      pView->PlayerCommand(CMD_MOVE_TO_START,0);
      
      m_iFramesPlayed = 0;     
      
      ui->position->setRange(0,(pDoc->GetFrameCount()*pDoc->GetPatternLength()));
      ui->position->setPageStep(pDoc->GetPatternLength());
   }
}

void MainWindow::current_returnPressed()
{
   if ( ui->current->findText(ui->current->lineEdit()->text()) == -1 )
   {
      ui->current->setCurrentIndex(ui->current->currentIndex());
   }
}

void MainWindow::on_visuals_toggled(bool checked)
{
   QSettings settings(QSettings::IniFormat, QSettings::UserScope, "CSPSoftware", "FamiPlayer");
   int visualsHeight = settings.value("VisualsHeight",100).toInt();
   
   settings.setValue("Visuals",checked);
   if ( checked )
   {
      setMaximumSize(16777215,16777215);
      setBaseSize(rect().width(),visualsHeight+rect().height());
   }
   else
   {
      settings.setValue("VisualsHeight",ui->sampleWindow->rect().height());
      setFixedHeight(minimumHeight());
   }
   
   ui->sampleWindow->setVisible(checked);
   
   QWidget *w = ui->sampleWindow->parentWidget();
   while (w) {
     w->adjustSize();
     w = w->parentWidget();
   }
}

void MainWindow::createShuffleLists()
{
   int path;
   int song;
   int subtune;
   int idx1, idx2;
   int moveIdx;
   QString pathSave;
   QString songSave;
   
   m_shuffleListFolder.clear();
   m_shuffleListSong.clear();
   
   for ( path = 0; path < ui->paths->count(); path++ )
   {
      QDir dir(ui->paths->itemText(path));
      QFileInfoList fileInfos = dir.entryInfoList(QStringList("*.ftm"));
      for ( song = 0; song < fileInfos.count(); song++ )
      {
         m_shuffleListFolder.append(ui->paths->itemText(path));
         m_shuffleListSong.append(fileInfos.at(song).fileName());
      }
   }

   // Shuffle seven times for good luck.   
   for ( idx1 = 0; idx1 < 7; idx1++ )
   {
      for ( idx2 = 0; idx2 < m_shuffleListFolder.count(); idx2++ )
      {
         moveIdx = rand()%m_shuffleListFolder.count();
         pathSave = m_shuffleListFolder.takeAt(0);
         m_shuffleListFolder.insert(moveIdx,pathSave);
         songSave = m_shuffleListSong.takeAt(0);
         m_shuffleListSong.insert(moveIdx,songSave);
      }
   }
   
   if ( m_shuffleListFolder.count() )
   {
      m_iCurrentShuffleIndex = rand()%m_shuffleListFolder.count();
   }
   else
   {
      m_iCurrentShuffleIndex = 0;
   }
}

void MainWindow::on_playOnStart_toggled(bool checked)
{
   QSettings settings(QSettings::IniFormat, QSettings::UserScope, "CSPSoftware", "FamiPlayer");
   settings.setValue("PlayOnStart",checked);
}

void MainWindow::on_repeat_toggled(bool checked)
{
   QSettings settings(QSettings::IniFormat, QSettings::UserScope, "CSPSoftware", "FamiPlayer");
   settings.setValue("Repeat",checked);    
}

void MainWindow::on_shuffle_toggled(bool checked)
{
   QSettings settings(QSettings::IniFormat, QSettings::UserScope, "CSPSoftware", "FamiPlayer");
   settings.setValue("Shuffle",checked);
   if ( checked )
   {
      createShuffleLists();
      on_next_clicked();
   }
}

void MainWindow::on_timeLimit_toggled(bool checked)
{
   QSettings settings(QSettings::IniFormat, QSettings::UserScope, "CSPSoftware", "FamiPlayer");
   settings.setValue("TimeLimit",checked);
}

void MainWindow::updateUiFromINI()
{
   QSettings settings(QSettings::IniFormat, QSettings::UserScope, "CSPSoftware", "FamiPlayer");
   QStringList folderList = settings.value("FolderList").toStringList();
   QString currentFolder = settings.value("CurrentFolder").toString();
   QString currentFile = settings.value("CurrentFile").toString();
   
   // Randomizeme!
   srand(QDateTime::currentDateTime().toTime_t());
   
   ui->paths->clear();
   ui->current->clear();

   if ( !folderList.isEmpty() )
   {
      ui->paths->addItems(folderList);
   }
   
   if ( !currentFolder.isEmpty() )
   {
      ui->paths->setCurrentIndex(ui->paths->findText(currentFolder));
   }

   if ( !currentFile.isEmpty() )
   {
      QFileInfo fileInfo(currentFile);
      ui->current->setCurrentIndex(ui->current->findText(fileInfo.fileName()));
   }   
   
   ui->info->setText("No playlist loaded.");
   ui->browse->setEnabled(true);
}

void MainWindow::updateUiFromPlaylist()
{
   QSettings settings(QSettings::IniFormat, QSettings::UserScope, "CSPSoftware", "FamiPlayer");
   
   if ( !settings.value("PlaylistFile").toString().isEmpty() )
   {
      PlaylistEditorDialog ped;
      QStringList files = ped.playlist();
      
      ui->paths->clear();
      ui->current->clear();
      foreach ( QString file, files )
      {
         QFileInfo fileInfo(file);
         if ( fileInfo.exists() )
         {
            if ( ui->paths->findText(fileInfo.path()) == -1 )
            {
               ui->paths->addItem(fileInfo.path());            
            }
            ui->current->addItem(fileInfo.fileName(),fileInfo.filePath());
         }
      }
      ui->info->setText("Playlist: "+settings.value("PlaylistFile").toString());
      ui->browse->setEnabled(false);
   }
   else
   {
      ui->info->setText("No playlist loaded.");
      ui->browse->setEnabled(true);
   }
}

void MainWindow::on_help_clicked()
{
   AboutDialog ad;
   ad.exec();
}

void MainWindow::on_playlist_clicked()
{
   QSettings settings(QSettings::IniFormat, QSettings::UserScope, "CSPSoftware", "FamiPlayer");
   bool wasPlaying = m_bPlaying;
   
   PlaylistEditorDialog ped;
   
   if ( wasPlaying )
   {
      on_playStop_clicked();
   }

   ped.exec();
   if ( settings.value("PlaylistFile").toString().isEmpty() )
   {
      updateUiFromINI();
   }
   else
   {
      updateUiFromPlaylist();               
   }
   
   if ( wasPlaying )
   {
      on_playStop_clicked();
   }
}
