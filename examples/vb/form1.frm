VERSION 5.00
Object = "{BDC217C8-ED16-11CD-956C-0000C04E4C0A}#1.1#0"; "TABCTL32.OCX"
Begin VB.Form Form1 
   BorderStyle     =   1  'Fixed Single
   Caption         =   "Seal 1.03 - Visual Basic Interface"
   ClientHeight    =   5220
   ClientLeft      =   2220
   ClientTop       =   2205
   ClientWidth     =   6690
   ControlBox      =   0   'False
   LinkTopic       =   "Form1"
   MaxButton       =   0   'False
   PaletteMode     =   1  'UseZOrder
   ScaleHeight     =   5220
   ScaleWidth      =   6690
   Begin VB.CommandButton Command3 
      Caption         =   "Quit"
      Height          =   375
      Left            =   5880
      TabIndex        =   12
      Top             =   0
      Width           =   735
   End
   Begin VB.TextBox Text1 
      Appearance      =   0  'Flat
      BackColor       =   &H80000004&
      BorderStyle     =   0  'None
      Enabled         =   0   'False
      BeginProperty Font 
         Name            =   "MS Sans Serif"
         Size            =   8.25
         Charset         =   0
         Weight          =   700
         Underline       =   0   'False
         Italic          =   0   'False
         Strikethrough   =   0   'False
      EndProperty
      Height          =   285
      Left            =   1440
      TabIndex        =   9
      Text            =   "None"
      Top             =   120
      Width           =   4215
   End
   Begin TabDlg.SSTab SSTab1 
      Height          =   4455
      Left            =   360
      TabIndex        =   0
      Top             =   480
      Width           =   6015
      _ExtentX        =   10610
      _ExtentY        =   7858
      _Version        =   327680
      Tab             =   2
      TabHeight       =   520
      TabCaption(0)   =   "Song Selector"
      TabPicture(0)   =   "Form1.frx":0000
      Tab(0).ControlCount=   3
      Tab(0).ControlEnabled=   0   'False
      Tab(0).Control(0)=   "File1"
      Tab(0).Control(0).Enabled=   0   'False
      Tab(0).Control(1)=   "Drive1"
      Tab(0).Control(1).Enabled=   0   'False
      Tab(0).Control(2)=   "Dir1"
      Tab(0).Control(2).Enabled=   0   'False
      TabCaption(1)   =   "PlayBack Controls"
      TabPicture(1)   =   "Form1.frx":001C
      Tab(1).ControlCount=   6
      Tab(1).ControlEnabled=   0   'False
      Tab(1).Control(0)=   "Label2"
      Tab(1).Control(0).Enabled=   0   'False
      Tab(1).Control(1)=   "Frame1"
      Tab(1).Control(1).Enabled=   0   'False
      Tab(1).Control(2)=   "Command2"
      Tab(1).Control(2).Enabled=   -1  'True
      Tab(1).Control(3)=   "PlayButton"
      Tab(1).Control(3).Enabled=   -1  'True
      Tab(1).Control(4)=   "Command1"
      Tab(1).Control(4).Enabled=   -1  'True
      Tab(1).Control(5)=   "StopButton"
      Tab(1).Control(5).Enabled=   -1  'True
      TabCaption(2)   =   "Author Info"
      TabPicture(2)   =   "Form1.frx":0038
      Tab(2).ControlCount=   4
      Tab(2).ControlEnabled=   -1  'True
      Tab(2).Control(0)=   "Label4"
      Tab(2).Control(0).Enabled=   0   'False
      Tab(2).Control(1)=   "Label3"
      Tab(2).Control(1).Enabled=   0   'False
      Tab(2).Control(2)=   "Command4"
      Tab(2).Control(2).Enabled=   0   'False
      Tab(2).Control(3)=   "Command5"
      Tab(2).Control(3).Enabled=   0   'False
      Begin VB.CommandButton Command5 
         Caption         =   "SEAL Page"
         Height          =   615
         Left            =   1200
         TabIndex        =   16
         Top             =   3240
         Width           =   3855
      End
      Begin VB.CommandButton Command4 
         Caption         =   "Egerter Software Home Page"
         Height          =   615
         Left            =   1200
         TabIndex        =   13
         Top             =   2040
         Width           =   3855
      End
      Begin VB.CommandButton StopButton 
         Caption         =   "Stop"
         Height          =   495
         Left            =   -72480
         TabIndex        =   7
         Top             =   2520
         Width           =   855
      End
      Begin VB.CommandButton Command1 
         Caption         =   "Next"
         Height          =   495
         Left            =   -71400
         TabIndex        =   6
         Top             =   1800
         Width           =   855
      End
      Begin VB.CommandButton PlayButton 
         Caption         =   "Play"
         Height          =   495
         Left            =   -72480
         TabIndex        =   5
         Top             =   1800
         Width           =   855
      End
      Begin VB.CommandButton Command2 
         Caption         =   "Prev"
         Height          =   495
         Left            =   -73560
         TabIndex        =   4
         Top             =   1800
         Width           =   855
      End
      Begin VB.DirListBox Dir1 
         Height          =   1440
         Left            =   -74280
         TabIndex        =   3
         Top             =   960
         Width           =   4695
      End
      Begin VB.DriveListBox Drive1 
         Height          =   315
         Left            =   -74280
         TabIndex        =   2
         Top             =   480
         Width           =   4695
      End
      Begin VB.FileListBox File1 
         Height          =   1650
         Left            =   -74280
         Pattern         =   "*.mod;*.s3m;*.mtm;*.xm"
         TabIndex        =   1
         Top             =   2520
         Width           =   4695
      End
      Begin VB.Frame Frame1 
         Caption         =   "Playback"
         Height          =   2175
         Left            =   -74280
         TabIndex        =   10
         Top             =   1200
         Width           =   4575
      End
      Begin VB.Label Label3 
         Caption         =   "SEAL by Carlos Hasan"
         BeginProperty Font 
            Name            =   "MS Sans Serif"
            Size            =   8.25
            Charset         =   0
            Weight          =   700
            Underline       =   0   'False
            Italic          =   0   'False
            Strikethrough   =   0   'False
         EndProperty
         Height          =   255
         Left            =   1440
         TabIndex        =   15
         Top             =   1080
         Width           =   3495
      End
      Begin VB.Label Label4 
         Caption         =   "Modifications to VB interface by Barry Egerter"
         Height          =   255
         Left            =   1440
         TabIndex        =   14
         Top             =   1320
         Width           =   3615
      End
      Begin VB.Label Label2 
         Caption         =   "Use this simple playback system to control the music."
         Height          =   255
         Left            =   -74160
         TabIndex        =   11
         Top             =   720
         Width           =   4335
      End
   End
   Begin VB.Label Label1 
      Caption         =   "Current Song:"
      Height          =   255
      Left            =   360
      TabIndex        =   8
      Top             =   120
      Width           =   1095
   End
End
Attribute VB_Name = "Form1"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = False
Attribute VB_PredeclaredId = True
Attribute VB_Exposed = False
Dim iewindow As InternetExplorer
Dim szFileName As String * 256
Dim lpModule As Long
Dim bSongPlaying As Long
  
  
Private Sub Command1_Click()
  
  Dim pnOrder As Long
  Dim lpnRow As Long

  If AGetModulePosition(pnOrder, lpnRow) <> AUDIO_ERROR_NONE Then
    Exit Sub
  End If
  
  If ASetModulePosition(pnOrder + 1, 0) <> AUDIO_ERROR_NONE Then
    Exit Sub
  End If
  
End Sub

Private Sub Command2_Click()
  
  Dim pnOrder As Long
  Dim lpnRow As Long

  If AGetModulePosition(pnOrder, lpnRow) <> AUDIO_ERROR_NONE Then
    Exit Sub
  End If
  
  If pnOrder > 1 Then
    pnOrder = pnOrder - 1
  Else
    pnOrder = 0
  End If
  
  If ASetModulePosition(pnOrder, 0) <> AUDIO_ERROR_NONE Then
    Exit Sub
  End If
  
End Sub

Private Sub Command3_Click()

  StopButton_Click
  Form1.Hide
  Unload Form1

End Sub

Private Sub Command4_Click()
            
    Set iewindow = New InternetExplorer
    
    iewindow.Visible = True
    iewindow.Navigate ("http://www.egerter.com")
    
End Sub

Private Sub Command5_Click()
    
    Set iewindow = New InternetExplorer
        
    iewindow.Visible = True
    iewindow.Navigate ("http://www.egerter.com/seal")
    
End Sub

Private Sub Dir1_Change()
  
  File1.Path = Dir1.Path
  File1.Refresh
  
End Sub

Private Sub Drive1_Change()
  
  Dir1.Path = Drive1.Drive
  Dir1.Refresh
  
End Sub

Private Sub File1_Click()

  Text1.Text = Dir1.Path + "\" + File1.filename
  Text1.Refresh
  szFileName = Text1.Text
  StopButton_Click
  
  SSTab1.Tab = 1
  
End Sub

Private Sub PlayButton_Click()
  ' WARNING! It's the very first time I have ever used VB, after some hours
  ' I could finally write a sort of interface for AUDIOW32.DLL, it's not
  ' perfect, there are still some things I couldn't figure how to port.
  ' I used VB 4.0 and SEAL 1.03 to test this code.

  Dim Info As AudioInfo
  
  If bSongPlaying Then
    StopButton_Click
  End If
  
  ' set up audio configuration structure
  Info.nDeviceId = AUDIO_DEVICE_MAPPER
  Info.wFormat = AUDIO_FORMAT_STEREO + AUDIO_FORMAT_16BITS
  Info.nSampleRate = 22050 ' 44100 is an unsigned 16-bit integer!
  
  ' open the default audio device, return if error
  If AOpenAudio(Info) <> AUDIO_ERROR_NONE Then
    Exit Sub
  End If
    
  ' open 32 active voices
  If AOpenVoices(32) <> AUDIO_ERROR_NONE Then
    ACloseAudio
    Exit Sub
  End If
  
  ' load module file from disk, shutdown and return if error
  If ALoadModuleFile(szFileName, lpModule, 0) <> AUDIO_ERROR_NONE Then
    ACloseVoices
    ACloseAudio
    Exit Sub
  End If
    
  ' start playing the module file
  If APlayModule(lpModule) <> AUDIO_ERROR_NONE Then
    ACloseVoices
    ACloseAudio
    Exit Sub
  End If

  bSongPlaying = 1

End Sub

Private Sub StopButton_Click()

  If bSongPlaying = 0 Then
    Exit Sub
  End If
  
  bSongPlaying = 0
  
  ' stop playing the module file
  AStopModule
  
  ' release the module file
  AFreeModuleFile (lpModule)
  
  ' close audio device
  ACloseVoices
  ACloseAudio
  
End Sub
