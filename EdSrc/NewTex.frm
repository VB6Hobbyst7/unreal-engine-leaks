VERSION 5.00
Begin VB.Form frmNewTex 
   Caption         =   "Create a new texture"
   ClientHeight    =   2715
   ClientLeft      =   4695
   ClientTop       =   9135
   ClientWidth     =   4245
   Icon            =   "NewTex.frx":0000
   LinkTopic       =   "Form1"
   PaletteMode     =   1  'UseZOrder
   ScaleHeight     =   2715
   ScaleWidth      =   4245
   ShowInTaskbar   =   0   'False
   Begin VB.Frame Frame2 
      Caption         =   "Size"
      BeginProperty Font 
         Name            =   "Arial"
         Size            =   11.25
         Charset         =   0
         Weight          =   700
         Underline       =   0   'False
         Italic          =   -1  'True
         Strikethrough   =   0   'False
      EndProperty
      Height          =   795
      Left            =   60
      TabIndex        =   7
      Top             =   1380
      Width           =   4095
      Begin VB.ComboBox VSize 
         Height          =   315
         Left            =   2820
         Style           =   2  'Dropdown List
         TabIndex        =   11
         Top             =   300
         Width           =   1035
      End
      Begin VB.ComboBox USize 
         Height          =   315
         Left            =   900
         Style           =   2  'Dropdown List
         TabIndex        =   9
         Top             =   300
         Width           =   1035
      End
      Begin VB.Label Label4 
         Alignment       =   1  'Right Justify
         Caption         =   "V-Size:"
         Height          =   195
         Left            =   2040
         TabIndex        =   10
         Top             =   360
         Width           =   675
      End
      Begin VB.Label Label2 
         Alignment       =   1  'Right Justify
         Caption         =   "U-Size:"
         Height          =   195
         Left            =   120
         TabIndex        =   8
         Top             =   360
         Width           =   675
      End
   End
   Begin VB.Frame Frame1 
      Caption         =   "Texture"
      BeginProperty Font 
         Name            =   "Arial"
         Size            =   11.25
         Charset         =   0
         Weight          =   700
         Underline       =   0   'False
         Italic          =   -1  'True
         Strikethrough   =   0   'False
      EndProperty
      Height          =   1215
      Left            =   60
      TabIndex        =   2
      Top             =   60
      Width           =   4095
      Begin VB.TextBox Family 
         Height          =   315
         Left            =   900
         TabIndex        =   6
         Text            =   "Text1"
         Top             =   780
         Width           =   2955
      End
      Begin VB.TextBox Name 
         Height          =   315
         Left            =   900
         TabIndex        =   4
         Text            =   "Text1"
         Top             =   360
         Width           =   2955
      End
      Begin VB.Label Label3 
         Alignment       =   1  'Right Justify
         Caption         =   "Family:"
         Height          =   255
         Left            =   60
         TabIndex        =   5
         Top             =   840
         Width           =   735
      End
      Begin VB.Label Label1 
         Alignment       =   1  'Right Justify
         Caption         =   "Name:"
         Height          =   255
         Left            =   60
         TabIndex        =   3
         Top             =   420
         Width           =   735
      End
   End
   Begin VB.CommandButton Cancel 
      Cancel          =   -1  'True
      Caption         =   "&Cancel"
      Height          =   375
      Left            =   3300
      TabIndex        =   1
      Top             =   2280
      Width           =   855
   End
   Begin VB.CommandButton Ok 
      Caption         =   "C&reate this texture"
      Default         =   -1  'True
      Height          =   375
      Left            =   60
      TabIndex        =   0
      Top             =   2280
      Width           =   1815
   End
End
Attribute VB_Name = "frmNewTex"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = False
Attribute VB_PredeclaredId = True
Attribute VB_Exposed = False
Private Sub Cancel_Click()
    GResult = 0
    Unload Me
End Sub

Private Sub Form_Load()
    Call Ed.SetOnTop(Me, "NewClass", TOP_NORMAL)
End Sub

Private Sub Form_Unload(Cancel As Integer)
    Call Ed.EndOnTop(Me)
End Sub

Private Sub Ok_Click()
    GResult = 1
    GString = NewClassName.Text
    Unload Me
End Sub
