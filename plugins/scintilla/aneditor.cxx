/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
    aneditor.cxx
    Copyright (C) 2003 Naba Kumar

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/
/*
 * Most of the code stolen from SciTE and heavily modified.
 * If code sections are later imported from SciTE, utmost care
 * should be taken to ensure that it does not conflict with the present code.
 */

#include "aneditor-priv.h"

#include <libanjuta/anjuta-debug.h>

static void lowerCaseString(char *s);
static AnEditor * aneditor_get(AnEditorID id);

gint on_aneditor_focus_in(GtkWidget* widget, gpointer * unused, AnEditor* ed);
gint on_aneditor_focus_out(GtkWidget* widget, gpointer * unused, AnEditor* ed);

static const char *extList[] = {
    "", "x", "x.cpp", "x.cpp", "x.html", "x.xml", "x.js", "x.vbs", "x.mak", "x.java",
    "x.lua", "x.py", "x.pl", "x.sql", "x.spec", "x.php3", "x.tex", "x.diff", "x.pas",
	"x.cs", "x.properties", "x.conf", "x.bc", "x.adb", "x.lisp", "x.rb", ".m"
};

const AnEditorID ANE_ID_INVALID = G_MAXUINT;
AnEditorID AnEditor::focusedID = ANE_ID_INVALID;

AnEditor::AnEditor(PropSetFile* p) {

	characterSet = 0;
	language = "java";
	lexLanguage = SCLEX_CPP;
	/*
	indentSize = 8;
	indentOpening = true;
	indentClosing = true;
	statementLookback = 10;
	*/
	indentMaintain = true;
	
	wrapLine = true;
	isReadOnly = false;
	fnEditor = 0;
	ptrEditor = 0;
	fnOutput = 0;
	ptrOutput = 0;
	tbVisible = false;
	sbVisible = false;
	tabVisible = false;
	tabMultiLine = false;
	visHeightTools = 0;
	visHeightStatus = 0;
	visHeightEditor = 1;
	heightBar = 7;

	heightOutput = 0;

	allowMenuActions = true;
	isDirty = false;

	ptStartDrag.x = 0;
	ptStartDrag.y = 0;
	capturedMouse = false;
	firstPropertiesRead = true;
	splitVertical = false;
	bufferedDraw = true;
	bracesCheck = true;
	bracesSloppy = false;
	bracesStyle = 0;
	braceCount = 0;

	indentationWSVisible = true;

	autoCompleteIgnoreCase = false;
	callTipIgnoreCase = false;
	autoCCausedByOnlyOne = false;
//	startCalltipWord = 0;
 
	// init calltips
	SetCallTipDefaults( );
	call_tip_node_queue = g_queue_new();
	lastPos = 0;
		
	autocompletion = NULL;
	
	margin = false;
	marginWidth = marginWidthDefault;
	foldMargin = true;
	foldMarginWidth = foldMarginWidthDefault;
	lineNumbers = false;
	lineNumbersWidth = lineNumbersWidthDefault;
	usePalette = false;

	accelGroup = NULL;
	calltipShown = false;
	// debugTipOn = false;
	
	fileName[0] = '\0';
	props = p;
	
	wEditor = scintilla_new();
	g_object_ref (G_OBJECT (wEditor.GetID()));
	gtk_object_sink (GTK_OBJECT (wEditor.GetID()));
	scintilla_set_id(SCINTILLA(wEditor.GetID()), 0);

	fnEditor = reinterpret_cast<SciFnDirect>(Platform::SendScintilla(
		wEditor.GetID(), SCI_GETDIRECTFUNCTION, 0, 0));

	ptrEditor = Platform::SendScintilla(wEditor.GetID(), 
		SCI_GETDIRECTPOINTER, 0, 0);

	g_signal_connect(wEditor.GetID(), "sci-notify", G_CALLBACK(NotifySignal), this);

	/* We will handle all accels ourself */
	/* SendEditor(SCI_CLEARALLCMDKEYS); */

	/*We have got our own popup menu */
	SendEditor(SCI_USEPOPUP, false);
	/* Set default editor mode */
	SendEditor(SCI_SETEOLMODE, SC_EOL_LF);
	
#if 0
	// Trap 'TAB' key for automatic indentation.
	// SendEditor (SCI_ASSIGNCMDKEY, SCK_TAB, SCI_NULL);
	g_signal_connect (wEditor.GetID(), "key-press-event",
					  G_CALLBACK (KeyPressEvent), this);

	/* Register images to be used for autocomplete */
	typedef struct {
		SVNodeType type;
		char **xpm_data;
	} PixAndType;
	PixAndType pix_list[] = {
	  { sv_none_t , sv_unknown_xpm }
	, { sv_class_t, sv_class_xpm }
  , { sv_struct_t, sv_struct_xpm }
  , { sv_union_t, sv_struct_xpm }
	, { sv_function_t, sv_function_xpm }
	, { sv_macro_t, sv_macro_xpm }
  , { sv_variable_t, sv_variable_xpm }
  , { sv_private_func_t, sv_private_fun_xpm }
	, { sv_private_var_t, sv_private_var_xpm }
	, { sv_protected_func_t, sv_protected_fun_xpm }
	, { sv_protected_var_t, sv_protected_var_xpm }
	, { sv_public_func_t, sv_public_fun_xpm }
	, { sv_public_var_t, sv_public_var_xpm }
	};
	for (guint i = 0; i < (sizeof(pix_list)/sizeof(pix_list[0])); ++i)
	{
		SendEditor(SCI_REGISTERIMAGE, (long) pix_list[i].type
		  , reinterpret_cast<long>(pix_list[i].xpm_data));
	}
#endif
}

void AnEditor::SetParent(AnEditor *parent)
{
	long pdoc = parent->SendEditor(SCI_GETDOCPOINTER, 0, 0);
	SendEditor(SCI_SETDOCPOINTER, 0, pdoc);
}

#if 0
static SVNodeType
sv_get_node_type (TMTag *tag)
{
	char access;

	if (!tag || (tm_tag_file_t == tag->type))
		return sv_none_t;
  
	access = tag->atts.entry.access;
	switch (tag->type)
	{
		case tm_tag_class_t:
			return sv_class_t;
		case tm_tag_struct_t:
			return sv_struct_t;
    case tm_tag_union_t:
			return sv_union_t;
		case tm_tag_function_t:
		case tm_tag_prototype_t:
			switch (access)
			{
				case TAG_ACCESS_PRIVATE:
					return sv_private_func_t;
					
				case TAG_ACCESS_PROTECTED:
					return sv_protected_func_t;
					
				case TAG_ACCESS_PUBLIC:
					return sv_public_func_t;
					
				default:
					return sv_function_t;
			}
		case tm_tag_member_t:
			switch (access)
			{
				case TAG_ACCESS_PRIVATE:
					return sv_private_var_t;
					
				case TAG_ACCESS_PROTECTED:
					return sv_protected_var_t;

				case TAG_ACCESS_PUBLIC:
					return sv_public_var_t;
					
				default:
					return sv_variable_t;				
			}
	
		case tm_tag_externvar_t:
		case tm_tag_variable_t:
			return sv_variable_t;
			
		case tm_tag_macro_t:
		case tm_tag_macro_with_arg_t:
			return sv_macro_t;
			
		default:
			return sv_none_t;
			
	}
	
}
#endif

void
AnEditor::SetAccelGroup(GtkAccelGroup* acl) {
	accelGroup = acl;
}

AnEditor::~AnEditor() {
	g_object_unref (G_OBJECT (wEditor.GetID()));
}

long AnEditor::SendEditor(unsigned int msg, unsigned long wParam, long lParam) {
	return fnEditor(ptrEditor, msg, wParam, lParam);
}

long AnEditor::SendEditorString(unsigned int msg, unsigned long wParam, const char *s) {
	return SendEditor(msg, wParam, reinterpret_cast<long>(s));
}

void AnEditor::ViewWhitespace(bool view) {
	if (view && indentationWSVisible)
		SendEditor(SCI_SETVIEWWS, SCWS_VISIBLEALWAYS);
	else if (view)
		SendEditor(SCI_SETVIEWWS, SCWS_VISIBLEAFTERINDENT);
	else
		SendEditor(SCI_SETVIEWWS, SCWS_INVISIBLE);
}

StyleAndWords AnEditor::GetStyleAndWords(const char *base) {
	StyleAndWords sw;
	SString fileNameForExtension = ExtensionFileName();
	SString sAndW = props->GetNewExpand(base, fileNameForExtension.c_str());
	sw.styleNumber = sAndW.value();
	const char *space = strchr(sAndW.c_str(), ' ');
	if (space)
		sw.words = space + 1;
	return sw;
}

void AnEditor::AssignKey(int key, int mods, int cmd) {
	SendEditor(SCI_ASSIGNCMDKEY,
	           Platform::LongFromTwoShorts(static_cast<short>(key),
	                                       static_cast<short>(mods)), cmd);
}

void AnEditor::SetOverrideLanguage(int ID) {
	overrideExtension = extList[ID - TE_LEXER_BASE];
}

int AnEditor::LengthDocument() {
	return SendEditor(SCI_GETLENGTH);
}

int AnEditor::GetCaretInLine() {
	int caret = SendEditor(SCI_GETCURRENTPOS);
	int line = SendEditor(SCI_LINEFROMPOSITION, caret);
	int lineStart = SendEditor(SCI_POSITIONFROMLINE, line);
	return caret - lineStart;
}

void AnEditor::GetLine(SString & text, int line) {
	if (line < 0)
		line = GetCurrentLineNumber();
	int lineStart = SendEditor(SCI_POSITIONFROMLINE, line);
	int lineEnd = SendEditor(SCI_GETLINEENDPOSITION, line);
	int len = lineEnd - lineStart + 1;
	char *text_buffer = SString::StringAllocate (len);
	GetRange(wEditor, lineStart, lineEnd, text_buffer);
	text_buffer[len] = '\0';
	text.attach(text_buffer, len);
}

int AnEditor::GetFullLine(SString & text, int line) {
	int caret, lineStart, lineEnd;
	if (line < 0)
	{
		line = GetCurrentLineNumber();
		caret = GetCaretInLine();
		lineStart = SendEditor(SCI_POSITIONFROMLINE, line);
		lineEnd = SendEditor(SCI_GETLINEENDPOSITION, line);
	}
	else
	{
		lineStart = SendEditor(SCI_POSITIONFROMLINE, line);
		lineEnd = SendEditor(SCI_GETLINEENDPOSITION, line);
		caret = lineEnd - lineStart - 1;
	}
	int count = 25, current; 
	int len =  lineEnd - lineStart + 1;
	text.clear();
	while(count)
	{
		{
			char *text_buffer = SString::StringAllocate (len + text.length());
			GetRange(wEditor, lineStart, lineEnd, text_buffer);
			memcpy(text_buffer + len - 1, text.c_str(), text.length());
			text_buffer[len + text.length()] = '\0';
			text.attach(text_buffer, len + text.length());
		}
		current = caret;
		while(current > 0)
		{
			if(text[current - 1] == ';' ||
			   text[current - 1] == '{' || text[current - 1] == '}')
			{
				return caret;
			}
			current--;
		}	
		line--;
		if(line < 0) break;
		lineStart = SendEditor(SCI_POSITIONFROMLINE, line);
		lineEnd = SendEditor(SCI_GETLINEENDPOSITION, line);
		len =  lineEnd - lineStart;	
		caret += len;
		len++;
		count--;
	}
	text.clear();
	return -1;
}


void AnEditor::GetRange(Window &win, int start, int end, char *text) {
	TextRange tr;
	tr.chrg.cpMin = start;
	tr.chrg.cpMax = end;
	tr.lpstrText = text;
	Platform::SendScintilla(win.GetID(), SCI_GETTEXTRANGE, 0, reinterpret_cast<long>(&tr));
}

void AnEditor::GetRange(guint start, guint end, gchar *text, gboolean styled) {
	TextRange tr;
	tr.chrg.cpMin = start;
	tr.chrg.cpMax = end;
	tr.lpstrText = text;
	if (styled)
		SendEditor (SCI_GETSTYLEDTEXT, 0, reinterpret_cast<long>(&tr));
	else
		SendEditor (SCI_GETTEXTRANGE, 0, reinterpret_cast<long>(&tr));
}

#if 0

/**
 * Check if the given line is a preprocessor condition line.
 * @return The kind of preprocessor condition (enum values).
 */
int AnEditor::IsLinePreprocessorCondition(const char *line) {
	const char *currChar = line;
	char word[32];
	int i = 0;

	if (!currChar) {
		return false;
	}
	while (isspacechar(*currChar) && *currChar) {
		currChar++;
	}
	if (preprocessorSymbol && (*currChar == preprocessorSymbol)) {
		currChar++;
		while (isspacechar(*currChar) && *currChar) {
			currChar++;
		}
		while (!isspacechar(*currChar) && *currChar) {
			word[i++] = *currChar++;
		}
		word[i] = '\0';
		if (preprocCondStart.InList(word)) {
			return ppcStart;
		}
		if (preprocCondMiddle.InList(word)) {
			return ppcMiddle;
		}
		if (preprocCondEnd.InList(word)) {
			return ppcEnd;
		}
	}
	return noPPC;
}

/**
 * Search a matching preprocessor condition line.
 * @return @c true if the end condition are meet.
 * Also set curLine to the line where one of these conditions is mmet.
 */
bool AnEditor::FindMatchingPreprocessorCondition(
    int &curLine,  		///< Number of the line where to start the search
    int direction,  		///< Direction of search: 1 = forward, -1 = backward
    int condEnd1,  		///< First status of line for which the search is OK
    int condEnd2) {		///< Second one

	bool isInside = false;
	SString line;
	int status, level = 0;
	int maxLines = SendEditor(SCI_GETLINECOUNT);

	while (curLine < maxLines && curLine > 0 && !isInside) {
		curLine += direction;	// Increment or decrement
		GetLine(line, curLine);
		status = IsLinePreprocessorCondition(line.c_str());

		if ((direction == 1 && status == ppcStart) || (direction == -1 && status == ppcEnd)) {
			level++;
		} else if (level > 0 && ((direction == 1 && status == ppcEnd) || (direction == -1 && status == ppcStart))) {
			level--;
		} else if (level == 0 && (status == condEnd1 || status == condEnd2)) {
			isInside = true;
		}
	}

	return isInside;
}

/**
 * Find if there is a preprocessor condition after or before the caret position,
 * @return @c true if inside a preprocessor condition.
 */
#ifdef __BORLANDC__
// Borland warns that isInside is assigned a value that is never used in this method.
// This is OK so turn off the warning just for this method.
#pragma warn -aus
#endif
bool AnEditor::FindMatchingPreprocCondPosition(
    bool isForward,  		///< @c true if search forward
    int &mppcAtCaret,  	///< Matching preproc. cond.: current position of caret
    int &mppcMatch) {	///< Matching preproc. cond.: matching position

	bool isInside = false;
	int curLine;
	SString line;
	int status;

	// Get current line
	curLine = SendEditor(SCI_LINEFROMPOSITION, mppcAtCaret);
	GetLine(line, curLine);
	status = IsLinePreprocessorCondition(line.c_str());

	switch (status) {
	case ppcStart:
		if (isForward) {
			isInside = FindMatchingPreprocessorCondition(curLine, 1, ppcMiddle, ppcEnd);
		} else {
			mppcMatch = mppcAtCaret;
			return true;
		}
		break;
	case ppcMiddle:
		if (isForward) {
			isInside = FindMatchingPreprocessorCondition(curLine, 1, ppcMiddle, ppcEnd);
		} else {
			isInside = FindMatchingPreprocessorCondition(curLine, -1, ppcStart, ppcMiddle);
		}
		break;
	case ppcEnd:
		if (isForward) {
			mppcMatch = mppcAtCaret;
			return true;
		} else {
			isInside = FindMatchingPreprocessorCondition(curLine, -1, ppcStart, ppcMiddle);
		}
		break;
	default:  	// Should be noPPC

		if (isForward) {
			isInside = FindMatchingPreprocessorCondition(curLine, 1, ppcMiddle, ppcEnd);
		} else {
			isInside = FindMatchingPreprocessorCondition(curLine, -1, ppcStart, ppcMiddle);
		}
		break;
	}

	if (isInside) {
		mppcMatch = SendEditor(SCI_POSITIONFROMLINE, curLine);
	}
	return isInside;
}
#endif

#ifdef __BORLANDC__
#pragma warn .aus
#endif

/**
 * Find if there is a brace next to the caret, checking before caret first, then
 * after caret. If brace found also find its matching brace.
 * @return @c true if inside a bracket pair.
 */
bool AnEditor::FindMatchingBracePosition(bool editor, int &braceAtCaret, int &braceOpposite, bool sloppy) {
	bool isInside = false;
	// Window &win = editor ? wEditor : wOutput;
	Window &win = wEditor;
	int bracesStyleCheck = editor ? bracesStyle : 0;
	int caretPos = Platform::SendScintilla(win.GetID(), SCI_GETCURRENTPOS, 0, 0);
	braceAtCaret = -1;
	braceOpposite = -1;
	char charBefore = '\0';
	char styleBefore = '\0';
	WindowAccessor acc(win.GetID(), *props);
	if (caretPos > 0) {
		charBefore = acc[caretPos - 1];
		styleBefore = static_cast<char>(acc.StyleAt(caretPos - 1) & 31);
	}
	// Priority goes to character before caret
	if (charBefore && strchr("[](){}", charBefore) &&
			((styleBefore == bracesStyleCheck) || (!bracesStyle))) {
		braceAtCaret = caretPos - 1;
	}
	bool colonMode = false;
	if (lexLanguage == SCLEX_PYTHON && ':' == charBefore) {
		braceAtCaret = caretPos - 1;
		colonMode = true;
	}
	bool isAfter = true;
	if (sloppy && (braceAtCaret < 0)) {
		// No brace found so check other side
		char charAfter = acc[caretPos];
		char styleAfter = static_cast<char>(acc.StyleAt(caretPos) & 31);
		if (charAfter && strchr("[](){}", charAfter) && (styleAfter == bracesStyleCheck)) {
			braceAtCaret = caretPos;
			isAfter = false;
		}
		if (lexLanguage == SCLEX_PYTHON && ':' == charAfter) {
			braceAtCaret = caretPos;
			colonMode = true;
		}
	}
	if (braceAtCaret >= 0) {
		if (colonMode) {
			int lineStart = Platform::SendScintilla(win.GetID(), SCI_LINEFROMPOSITION, braceAtCaret);
			int lineMaxSubord = Platform::SendScintilla(win.GetID(), SCI_GETLASTCHILD, lineStart, -1);
			braceOpposite = Platform::SendScintilla(win.GetID(), SCI_GETLINEENDPOSITION, lineMaxSubord);
		} else {
			braceOpposite = Platform::SendScintilla(win.GetID(), SCI_BRACEMATCH, braceAtCaret, 0);
		}
		if (braceOpposite > braceAtCaret) {
			isInside = isAfter;
		} else {
			isInside = !isAfter;
		}
	}
	return isInside;
}

void AnEditor::BraceMatch(bool editor) {
	if (!bracesCheck)
		return;
	int braceAtCaret = -1;
	int braceOpposite = -1;
	FindMatchingBracePosition(editor, braceAtCaret, braceOpposite, bracesSloppy);
	// Window &win = editor ? wEditor : wOutput;
	Window &win = wEditor;
	if ((braceAtCaret != -1) && (braceOpposite == -1)) {
		Platform::SendScintilla(win.GetID(), SCI_BRACEBADLIGHT, braceAtCaret, 0);
		SendEditor(SCI_SETHIGHLIGHTGUIDE, 0);
	} else {
		char chBrace = static_cast<char>(Platform::SendScintilla(
		                                         win.GetID(), SCI_GETCHARAT, braceAtCaret, 0));
		Platform::SendScintilla(win.GetID(), SCI_BRACEHIGHLIGHT, braceAtCaret, braceOpposite);
		int columnAtCaret = Platform::SendScintilla(win.GetID(), SCI_GETCOLUMN, braceAtCaret, 0);
		int columnOpposite = Platform::SendScintilla(win.GetID(), SCI_GETCOLUMN, braceOpposite, 0);
		if (chBrace == ':') {
			int lineStart = Platform::SendScintilla(win.GetID(), SCI_LINEFROMPOSITION, braceAtCaret);
			int indentPos = Platform::SendScintilla(win.GetID(), SCI_GETLINEINDENTPOSITION, lineStart, 0);
			int indentPosNext = Platform::SendScintilla(win.GetID(), SCI_GETLINEINDENTPOSITION, lineStart + 1, 0);
			columnAtCaret = Platform::SendScintilla(win.GetID(), SCI_GETCOLUMN, indentPos, 0);
			int columnAtCaretNext = Platform::SendScintilla(win.GetID(), SCI_GETCOLUMN, indentPosNext, 0);
			int indentationSize = Platform::SendScintilla(win.GetID(), SCI_GETINDENT);
			if (columnAtCaretNext - indentationSize > 1)
				columnAtCaret = columnAtCaretNext - indentationSize;
			//Platform::DebugPrintf(": %d %d %d\n", lineStart, indentPos, columnAtCaret);
			if (columnOpposite == 0)	// If the final line of the structure is empty
				columnOpposite = columnAtCaret;
		}

		if (props->GetInt("highlight.indentation.guides"))
			Platform::SendScintilla(win.GetID(), SCI_SETHIGHLIGHTGUIDE, Platform::Minimum(columnAtCaret, columnOpposite), 0);
	}
}

CharacterRange AnEditor::GetSelection() {
	CharacterRange crange;
	crange.cpMin = SendEditor(SCI_GETSELECTIONSTART);
	crange.cpMax = SendEditor(SCI_GETSELECTIONEND);
	return crange;
}

void AnEditor::SetSelection(int anchor, int currentPos) {
	SendEditor(SCI_SETSEL, anchor, currentPos);
}

bool AnEditor::iswordcharforsel(char ch) {
	return !strchr("\t\n\r !\"#$%&'()*+,-./:;<=>?@[\\]^`{|}~", ch);
}

void AnEditor::SelectionWord(char *word, int len) {
	int lengthDoc = LengthDocument();
	CharacterRange cr = GetSelection();
	int selStart = cr.cpMin;
	int selEnd = cr.cpMax;
	if (selStart == selEnd) {
		WindowAccessor acc(wEditor.GetID(), *props);
		// Try and find a word at the caret
		if (iswordcharforsel(acc[selStart])) {
			while ((selStart > 0) && (iswordcharforsel(acc[selStart - 1])))
				selStart--;
			while ((selEnd < lengthDoc - 1) && (iswordcharforsel(acc[selEnd + 1])))
				selEnd++;
			if (selStart < selEnd)
				selEnd++;   	// Because normal selections end one past
		}
	}
	word[0] = '\0';
	if ((selStart < selEnd) && ((selEnd - selStart + 1) < len)) {
		GetRange(wEditor, selStart, selEnd, word);
	}
}

void AnEditor::WordSelect() {
	int lengthDoc = LengthDocument();
	int selStart;
	int selEnd;
	
	selStart = selEnd = SendEditor(SCI_GETCURRENTPOS);
	WindowAccessor acc(wEditor.GetID(), *props);
	if (iswordcharforsel(acc[selStart])) {
			while ((selStart > 0) && (iswordcharforsel(acc[selStart - 1])))
				selStart--;
			while ((selEnd < lengthDoc - 1) && (iswordcharforsel(acc[selEnd + 1])))
				selEnd++;
			if (selStart < selEnd)
				selEnd++;   	// Because normal selections end one past
	}
	SetSelection(selStart, selEnd);
}

void AnEditor::LineSelect() {
	int pos = SendEditor(SCI_GETCURRENTPOS);
	int line = SendEditor(SCI_LINEFROMPOSITION, pos);
	int lineStart = SendEditor(SCI_POSITIONFROMLINE, line);
	int lineEnd = SendEditor(SCI_GETLINEENDPOSITION, line);
	
	SetSelection(lineStart, lineEnd);
}

void AnEditor::SelectionIntoProperties() {
	CharacterRange cr = GetSelection();
	char currentSelection[1000];
	if ((cr.cpMin < cr.cpMax) && ((cr.cpMax - cr.cpMin + 1) < static_cast<int>(sizeof(currentSelection)))) {
		GetRange(wEditor, cr.cpMin, cr.cpMax, currentSelection);
		int len = strlen(currentSelection);
		if (len > 2 && iscntrl(currentSelection[len - 1]))
			currentSelection[len - 1] = '\0';
		if (len > 2 && iscntrl(currentSelection[len - 2]))
			currentSelection[len - 2] = '\0';
		props->Set("CurrentSelection", currentSelection);
	}
	char word[200];
	SelectionWord(word, sizeof(word));
	props->Set("CurrentWord", word);
}

long AnEditor::Find (long flags, char* findWhat) {
	if (!findWhat) return -1;
	TextToFind ft = {{0, 0}, 0, {0, 0}};
	CharacterRange crange = GetSelection();
	if (flags & ANEFIND_REVERSE_FLAG) {
		ft.chrg.cpMin = crange.cpMin - 1;
		ft.chrg.cpMax = 1;
	} else {
		ft.chrg.cpMin = crange.cpMax;
		ft.chrg.cpMax = LengthDocument();
	}
	ft.lpstrText = findWhat;
	ft.chrgText.cpMin = 0;
	ft.chrgText.cpMax = 0;
	int posFind = SendEditor(SCI_FINDTEXT, flags, reinterpret_cast<long>(&ft));
	if (posFind >= 0) {
		EnsureRangeVisible(ft.chrgText.cpMin, ft.chrgText.cpMax);
		SetSelection(ft.chrgText.cpMin, ft.chrgText.cpMax);
	}
	return posFind;
}

void AnEditor::BookmarkToggle( int lineno ) {
	if (lineno == -1)
		
		
		
		
		
		lineno = GetCurrentLineNumber();
	int state = SendEditor(SCI_MARKERGET, lineno);
	if ( state & (1 << ANE_MARKER_BOOKMARK))
		SendEditor(SCI_MARKERDELETE, lineno, ANE_MARKER_BOOKMARK);
	else {
		SendEditor(SCI_MARKERADD, lineno, ANE_MARKER_BOOKMARK);
	}
}

void AnEditor::BookmarkFirst() {
	int lineno = GetCurrentLineNumber();
	int nextLine = SendEditor(SCI_MARKERNEXT, 0, 1 << ANE_MARKER_BOOKMARK);
	if (nextLine < 0 || nextLine == lineno)
		gdk_beep(); // how do I beep? -- like this ;-)
	else {
		SendEditor(SCI_ENSUREVISIBLE, nextLine);
		SendEditor(SCI_GOTOLINE, nextLine);
	}
}

void AnEditor::BookmarkPrev() {
	int lineno = GetCurrentLineNumber();
	int nextLine = SendEditor(SCI_MARKERPREVIOUS, lineno - 1, 1 << ANE_MARKER_BOOKMARK);
	if (nextLine < 0 || nextLine == lineno) {
		if(props->GetInt("editor.wrapbookmarks")) {
			int nrOfLines = SendEditor(SCI_GETLINECOUNT, 0, 1 << ANE_MARKER_BOOKMARK);
			int nextLine1 = SendEditor(SCI_MARKERPREVIOUS, nrOfLines, 1 << ANE_MARKER_BOOKMARK);
			if (nextLine1 < 0 || nextLine1 == lineno) {
				gdk_beep(); // how do I beep? -- like this ;-)
			} else {
				SendEditor(SCI_ENSUREVISIBLE, nextLine1);
				SendEditor(SCI_GOTOLINE, nextLine1);
			}
		}
	} else {
		SendEditor(SCI_ENSUREVISIBLE, nextLine);
		SendEditor(SCI_GOTOLINE, nextLine);
	}
}

void AnEditor::BookmarkNext() {
	int lineno = GetCurrentLineNumber();
	int nextLine = SendEditor(SCI_MARKERNEXT, lineno + 1, 1 << ANE_MARKER_BOOKMARK);
	if (nextLine < 0 || nextLine == lineno) {
		if(props->GetInt("editor.wrapbookmarks")) {
			int nextLine1 = SendEditor(SCI_MARKERNEXT, 0, 1 << ANE_MARKER_BOOKMARK);
			if (nextLine1 < 0 || nextLine1 == lineno) {
				gdk_beep(); // how do I beep? -- like this ;-)
			} else {				
				SendEditor(SCI_ENSUREVISIBLE, nextLine1);
				SendEditor(SCI_GOTOLINE, nextLine1);
			}
		}
	} else {
		SendEditor(SCI_ENSUREVISIBLE, nextLine);
		SendEditor(SCI_GOTOLINE, nextLine);
	}
}

void AnEditor::BookmarkLast() {
	int lineno = GetCurrentLineNumber();
	int nextLine = SendEditor(SCI_MARKERPREVIOUS,
		SendEditor(SCI_GETLINECOUNT), 1 << ANE_MARKER_BOOKMARK);
	if (nextLine < 0 || nextLine == lineno)
			gdk_beep(); // how do I beep? -- like this ;-)
	else {
		SendEditor(SCI_ENSUREVISIBLE, nextLine);
		SendEditor(SCI_GOTOLINE, nextLine);
	}
}

void AnEditor::BookmarkClear() {
	SendEditor(SCI_MARKERDELETEALL, ANE_MARKER_BOOKMARK);
}

bool AnEditor::GetCurrentWord(char* buffer, int length) {
	SString linebuf;
	GetLine(linebuf);
	int current = GetCaretInLine();
	return FindWordInRegion(buffer, length, linebuf, current);
}

bool AnEditor::StartBlockComment() {
	SString fileNameForExtension = ExtensionFileName();
	SString language = props->GetNewExpand("lexer.", fileNameForExtension.c_str());
	SString base("comment.block.");
	SString comment_at_line_start("comment.block.at.line.start.");
	base += language;
	comment_at_line_start += language;
	SString comment = props->Get(base.c_str());
	if (comment == "") { // user friendly error message box
		//SString error("Block comment variable \"");
		//error += base;
		//error += "\" is not defined in SciTE *.properties!";
		//WindowMessageBox(wEditor, error, MB_OK | MB_ICONWARNING);
		return true;
	}
	comment += " ";
	SString long_comment = comment;
	char linebuf[1000];
	size_t comment_length = comment.length();
	size_t selectionStart = SendEditor(SCI_GETSELECTIONSTART);
	size_t selectionEnd = SendEditor(SCI_GETSELECTIONEND);
	size_t caretPosition = SendEditor(SCI_GETCURRENTPOS);
	// checking if caret is located in _beginning_ of selected block
	bool move_caret = caretPosition < selectionEnd;
	int selStartLine = SendEditor(SCI_LINEFROMPOSITION, selectionStart);
	int selEndLine = SendEditor(SCI_LINEFROMPOSITION, selectionEnd);
	int lines = selEndLine - selStartLine;
	size_t firstSelLineStart = SendEditor(SCI_POSITIONFROMLINE, selStartLine);
	// "caret return" is part of the last selected line
	if ((lines > 0) &&
		(selectionEnd == static_cast<size_t>(SendEditor(SCI_POSITIONFROMLINE, selEndLine))))
		selEndLine--;
	SendEditor(SCI_BEGINUNDOACTION);
	for (int i = selStartLine; i <= selEndLine; i++) {
		int lineStart = SendEditor(SCI_POSITIONFROMLINE, i);
		int lineIndent = lineStart;
		int lineEnd = SendEditor(SCI_GETLINEENDPOSITION, i);
		if (props->GetInt(comment_at_line_start.c_str())) {
			GetRange(wEditor, lineIndent, lineEnd, linebuf);
		} else {
			lineIndent = GetLineIndentPosition(i);
			GetRange(wEditor, lineIndent, lineEnd, linebuf);
		}
		// empty lines are not commented
		if (strlen(linebuf) < 1)
			continue;
		if (memcmp(linebuf, comment.c_str(), comment_length - 1) == 0) {
			if (memcmp(linebuf, long_comment.c_str(), comment_length) == 0) {
				// removing comment with space after it
				SendEditor(SCI_SETSEL, lineIndent, lineIndent + comment_length);
				SendEditorString(SCI_REPLACESEL, 0, "");
				if (i == selStartLine) // is this the first selected line?
					selectionStart -= comment_length;
				selectionEnd -= comment_length; // every iteration
				continue;
			} else {
				// removing comment _without_ space
				SendEditor(SCI_SETSEL, lineIndent, lineIndent + comment_length - 1);
				SendEditorString(SCI_REPLACESEL, 0, "");
				if (i == selStartLine) // is this the first selected line?
					selectionStart -= (comment_length - 1);
				selectionEnd -= (comment_length - 1); // every iteration
				continue;
			}
		}
		if (i == selStartLine) // is this the first selected line?
			selectionStart += comment_length;
		selectionEnd += comment_length; // every iteration
		SendEditorString(SCI_INSERTTEXT, lineIndent, long_comment.c_str());
	}
	// after uncommenting selection may promote itself to the lines
	// before the first initially selected line;
	// another problem - if only comment symbol was selected;
	if (selectionStart < firstSelLineStart) {
		if (selectionStart >= selectionEnd - (comment_length - 1))
			selectionEnd = firstSelLineStart;
		selectionStart = firstSelLineStart;
	}
	if (move_caret) {
		// moving caret to the beginning of selected block
		SendEditor(SCI_GOTOPOS, selectionEnd);
		SendEditor(SCI_SETCURRENTPOS, selectionStart);
	} else {
		SendEditor(SCI_SETSEL, selectionStart, selectionEnd);
	}
	SendEditor(SCI_ENDUNDOACTION);
	return true;
}

//  Return true if the selected zone can be commented
//  Return false if it cannot be commented  or has been uncommented
//	BOX_COMMENT : box_stream = true    STREAM_COMMENT : box_stream = false
//	Uncomment if the selected zone or the cursor is inside the comment
//
bool AnEditor::CanBeCommented(bool box_stream) {
	SString fileNameForExtension = ExtensionFileName();
	SString language = props->GetNewExpand("lexer.", fileNameForExtension.c_str());
	SString start_base("comment.box.start.");
	SString middle_base("comment.box.middle.");
	SString end_base("comment.box.end.");
	SString white_space(" ");
	start_base += language;
	middle_base += language;
	end_base += language;
	SString start_comment = props->Get(start_base.c_str());
	SString middle_cmt = props->Get(middle_base.c_str());
	SString end_comment = props->Get(end_base.c_str());
	start_comment += white_space;
	middle_cmt += white_space;
	white_space += end_comment;
	end_comment = white_space;
	size_t start_comment_length = start_comment.length();
	size_t end_comment_length = end_comment.length(); 
	size_t middle_cmt_length = middle_cmt.length();	
	SString start_base_stream ("comment.stream.start.");
	start_base_stream += language;
	SString end_base_stream ("comment.stream.end.");
	end_base_stream += language;
	SString end_comment_stream = props->Get(end_base_stream.c_str());
	SString white_space_stream(" ");
	//SString end_white_space_stream("  ");
	SString start_comment_stream = props->Get(start_base_stream.c_str());
	start_comment_stream += white_space_stream;
	size_t start_comment_stream_length = start_comment_stream.length();
	white_space_stream +=end_comment_stream;
	end_comment_stream = white_space_stream ;
	size_t end_comment_stream_length = end_comment_stream.length();
	
	char linebuf[1000]; 
	size_t selectionStart = SendEditor(SCI_GETSELECTIONSTART);
	size_t selectionEnd = SendEditor(SCI_GETSELECTIONEND);
	if (selectionStart == selectionEnd)
		return FALSE;
	int line = SendEditor(SCI_LINEFROMPOSITION, selectionStart);
	if (line < 0)
		return FALSE;
	bool start1 = false, start2 = false;
	bool end1 = false, end2 = false;
	int lineEnd1;
	if (box_stream)
		lineEnd1 = selectionStart + start_comment_length;
	else
		lineEnd1 = selectionStart + start_comment_stream_length +1;
	if (lineEnd1 > LengthDocument())
		lineEnd1 = LengthDocument();

	int lineStart1 = 0;
	size_t start_cmt  = 0, end_cmt = 0;
	int index = 0;
	// Find Backward StartComment
	while (line >= 0 && start1 == false && end1 == false)
	{
		lineStart1 = SendEditor(SCI_POSITIONFROMLINE, line);
		GetRange(wEditor, lineStart1, lineEnd1, linebuf);

		for (index = lineEnd1-lineStart1; index >= 0; index--)
		{
			if (end1= ((end_comment_length > 1 && !memcmp(linebuf+index,
				end_comment.c_str(), end_comment_length ))  
			    || (end_comment_stream_length > 0 && !memcmp(linebuf+index, 
			    end_comment_stream.c_str(), end_comment_stream_length))))
				break;
			if (start1=((start_comment_length > 1 && !memcmp(linebuf+index, 
				start_comment.c_str(), start_comment_length))
				|| (start_comment_stream_length > 0 && !memcmp(linebuf+index, 
			    start_comment_stream.c_str(), start_comment_stream_length)))) 
				break;
		}	
		line --; 
		lineEnd1= SendEditor(SCI_GETLINEENDPOSITION, line);
	}
	start_cmt = index + lineStart1;
	line = SendEditor(SCI_LINEFROMPOSITION, selectionEnd);
	if (box_stream)
		lineStart1 = selectionEnd - start_comment_length;
	else
		lineStart1 = selectionEnd - start_comment_stream_length;
	int last_line = SendEditor(SCI_GETLINECOUNT);
	// Find Forward EndComment
	while (line <= last_line && start2 == false && end2 == false) 
	{
		lineEnd1= SendEditor(SCI_GETLINEENDPOSITION, line);
		GetRange(wEditor, lineStart1, lineEnd1, linebuf);
		for (index = 0; index <= (lineEnd1-lineStart1); index++)
		{
			if (start2= ((start_comment_length > 1 && !memcmp(linebuf+index, 
				start_comment.c_str(), start_comment_length))
				|| (start_comment_stream_length > 0 && !memcmp(linebuf+index, 
			    start_comment_stream.c_str(), start_comment_stream_length)))) 
				break;
			if (end2= ((end_comment_length > 1 && !memcmp(linebuf+index, 
				end_comment.c_str(), end_comment_length ))
				|| (end_comment_stream_length > 0 && !memcmp(linebuf+index, 
			    end_comment_stream.c_str(), end_comment_stream_length))))
				break;
		}
		line ++;
		end_cmt = lineStart1 + index;
		lineStart1 = SendEditor(SCI_POSITIONFROMLINE, line);	
	}
	//  Uncomment
	if(start1 || end2)
	{
		if (start1 && end2)
		{
			SendEditor(SCI_BEGINUNDOACTION);
			if (box_stream)	// Box
			{
				SendEditor(SCI_SETSEL, start_cmt, start_cmt + 
				           start_comment_length);  
				end_cmt -= start_comment_length; 
			}
			else            // Stream
			{
				SendEditor(SCI_SETSEL, start_cmt, start_cmt + 
				           start_comment_stream_length); 
				end_cmt -= start_comment_stream_length;
			}
			SendEditorString(SCI_REPLACESEL, 0, "");
			line = SendEditor(SCI_LINEFROMPOSITION, start_cmt) + 1;
			last_line = SendEditor(SCI_LINEFROMPOSITION, end_cmt) ;
			for (int i = line; i < last_line; i++)
			{
				int s = SendEditor(SCI_POSITIONFROMLINE, i);
				int e = SendEditor(SCI_GETLINEENDPOSITION, i);					
				GetRange(wEditor, s, e, linebuf);
				if (!memcmp(linebuf, middle_cmt.c_str(), middle_cmt_length))
				{
					SendEditor(SCI_SETSEL, s, s + middle_cmt_length);
					SendEditorString(SCI_REPLACESEL, 0, "");
					end_cmt -= middle_cmt_length;
				}
			}
			if (box_stream) // Box
				SendEditor(SCI_SETSEL, end_cmt, end_cmt + end_comment_length);
			else			// Stream
				SendEditor(SCI_SETSEL, end_cmt, end_cmt + end_comment_stream_length);
			SendEditorString(SCI_REPLACESEL, 0, "");
			SendEditor(SCI_ENDUNDOACTION);
		}
		return false;
	}
	return true;	
}


bool AnEditor::StartBoxComment() {
	SString fileNameForExtension = ExtensionFileName();
	SString language = props->GetNewExpand("lexer.", fileNameForExtension.c_str());
	SString start_base("comment.box.start.");
	SString middle_base("comment.box.middle.");
	SString end_base("comment.box.end.");
	SString white_space(" ");
	start_base += language;
	middle_base += language;
	end_base += language;
	SString start_comment = props->Get(start_base.c_str());
	SString middle_comment = props->Get(middle_base.c_str());
	SString end_comment = props->Get(end_base.c_str());
	if (start_comment == "" || middle_comment == "" || end_comment == "") {
		//SString error("Box comment variables \"");
		//error += start_base;
		//error += "\", \"";
		//error += middle_base;
		//error += "\"\nand \"";
		//error += end_base;
		//error += "\" are not ";
		//error += "defined in SciTE *.properties!";
		//WindowMessageBox(wSciTE, error, MB_OK | MB_ICONWARNING);
		return true;
	}
	start_comment += white_space;
	middle_comment += white_space;
	white_space += end_comment;
	end_comment = white_space;
	size_t start_comment_length = start_comment.length();
	size_t middle_comment_length = middle_comment.length();
	size_t selectionStart = SendEditor(SCI_GETSELECTIONSTART);
	size_t selectionEnd = SendEditor(SCI_GETSELECTIONEND);
	size_t caretPosition = SendEditor(SCI_GETCURRENTPOS);
	// checking if caret is located in _beginning_ of selected block
	bool move_caret = caretPosition < selectionEnd;
	size_t selStartLine = SendEditor(SCI_LINEFROMPOSITION, selectionStart);
	size_t selEndLine = SendEditor(SCI_LINEFROMPOSITION, selectionEnd);
	size_t lines = selEndLine - selStartLine;
	// "caret return" is part of the last selected line
	if ((lines > 0) && (
		selectionEnd == static_cast<size_t>(SendEditor(SCI_POSITIONFROMLINE, selEndLine)))) {
		selEndLine--;
		lines--;
		// get rid of CRLF problems
		selectionEnd = SendEditor(SCI_GETLINEENDPOSITION, selEndLine);
	}
	//	Comment , Uncomment or Do Nothing
	if (CanBeCommented(true))
	{
		SendEditor(SCI_BEGINUNDOACTION);
		// first commented line (start_comment)
		int lineStart = SendEditor(SCI_POSITIONFROMLINE, selStartLine);
		SendEditorString(SCI_INSERTTEXT, lineStart, start_comment.c_str());
		selectionStart += start_comment_length;
		// lines between first and last commented lines (middle_comment)
		for (size_t i = selStartLine + 1; i <= selEndLine; i++) {
			lineStart = SendEditor(SCI_POSITIONFROMLINE, i);
			SendEditorString(SCI_INSERTTEXT, lineStart, middle_comment.c_str());
			selectionEnd += middle_comment_length;
		}
		// last commented line (end_comment)
		int lineEnd = SendEditor(SCI_GETLINEENDPOSITION, selEndLine);
		if (lines > 0) {
			SendEditorString(SCI_INSERTTEXT, lineEnd, "\n");
			SendEditorString(SCI_INSERTTEXT, lineEnd + 1, (end_comment.c_str() + 1));
		} else {
			SendEditorString(SCI_INSERTTEXT, lineEnd, end_comment.c_str());
		}
		selectionEnd += (start_comment_length);
		if (move_caret) {
			// moving caret to the beginning of selected block
			SendEditor(SCI_GOTOPOS, selectionEnd);
			SendEditor(SCI_SETCURRENTPOS, selectionStart);
		} else {
			SendEditor(SCI_SETSEL, selectionStart, selectionEnd);
		}
		SendEditor(SCI_ENDUNDOACTION);
	}
	return true;
}

bool AnEditor::StartStreamComment() {
	SString fileNameForExtension = ExtensionFileName();
	SString language = props->GetNewExpand("lexer.", fileNameForExtension.c_str());
	SString start_base("comment.stream.start.");
	SString end_base("comment.stream.end.");
	SString white_space(" ");
	//SString end_white_space("  ");
	start_base += language;
	end_base += language;
	SString start_comment = props->Get(start_base.c_str());
	SString end_comment = props->Get(end_base.c_str());
	if (start_comment == "" || end_comment == "") {
		//SString error("Stream comment variables \"");
		//error += start_base;
		//error += "\" and \n\"";
		//error += end_base;
		//error += "\" are not ";
		//error += "defined in SciTE *.properties!";
		//WindowMessageBox(wSciTE, error, MB_OK | MB_ICONWARNING);
		return true;
	}
	start_comment += white_space;
	white_space += end_comment;
	end_comment = white_space;
	size_t start_comment_length = start_comment.length();
	size_t selectionStart = SendEditor(SCI_GETSELECTIONSTART);
	size_t selectionEnd = SendEditor(SCI_GETSELECTIONEND);
	size_t caretPosition = SendEditor(SCI_GETCURRENTPOS);
	// checking if caret is located in _beginning_ of selected block
	bool move_caret = caretPosition < selectionEnd;
	// if there is no selection?
	if (selectionEnd - selectionStart <= 0) {
		int selLine = SendEditor(SCI_LINEFROMPOSITION, selectionStart);
		int lineIndent = GetLineIndentPosition(selLine);
		int lineEnd = SendEditor(SCI_GETLINEENDPOSITION, selLine);
		if (RangeIsAllWhitespace(lineIndent, lineEnd))
			return true; // we are not dealing with empty lines
		SString linebuf;
		GetLine(linebuf);
		int current = GetCaretInLine();
		// checking if we are not inside a word
		if (!wordCharacters.contains(linebuf[current]))
			return true; // caret is located _between_ words
		int startword = current;
		int endword = current;
		int start_counter = 0;
		int end_counter = 0;
		while (startword > 0 && wordCharacters.contains(linebuf[startword - 1])) {
			start_counter++;
			startword--;
		}
		// checking _beginning_ of the word
		if (startword == current)
			return true; // caret is located _before_ a word
		while (linebuf[endword + 1] != '\0' && wordCharacters.contains(linebuf[endword + 1])) {
			end_counter++;
			endword++;
		}
		selectionStart -= start_counter;
		selectionEnd += (end_counter + 1);
	}
	//	Comment , Uncomment or Do Nothing
	if (CanBeCommented(false))
	{
		SendEditor(SCI_BEGINUNDOACTION);
		SendEditorString(SCI_INSERTTEXT, selectionStart, start_comment.c_str());
		selectionEnd += start_comment_length;
		selectionStart += start_comment_length;
		SendEditorString(SCI_INSERTTEXT, selectionEnd, end_comment.c_str());
		if (move_caret) {
			// moving caret to the beginning of selected block
			SendEditor(SCI_GOTOPOS, selectionEnd);
			SendEditor(SCI_SETCURRENTPOS, selectionStart);
		} else {
			SendEditor(SCI_SETSEL, selectionStart, selectionEnd);
		}
		SendEditor(SCI_ENDUNDOACTION);
	}
	return true;
}

#if 0
SString AnEditor::GetMode(SString language) {
	SString mode;	
	if (strcmp(language.c_str(), "cpp") == 0)
	{
		mode += " Mode: C;";
		if (props->GetInt("use.tabs"))
			mode += " indent-tabs-mode: t;";
		mode += " c-basic-offset: ";
		mode +=  g_strdup_printf("%d", props->GetInt("indent.size"));
		mode += "; tab-width: ";
		mode +=  g_strdup_printf("%d ", props->GetInt("tabsize"));
	}
//~ Other languages
//~ .....
	return mode;
}

/*	Insert or Modify a Comment line 
	giving File indent  */
bool AnEditor::InsertCustomIndent() {
	#define MAXBUF 1000
	
	SString fileNameForExtension = ExtensionFileName();
	SString language = props->GetNewExpand("lexer.", fileNameForExtension.c_str());
	SString start_box_base("comment.box.start.");
	start_box_base += language;
	SString start_stream_base("comment.stream.start.");
	start_stream_base += language;
	SString end_box_base("comment.box.end.");
	end_box_base += language;
	SString end_stream_base("comment.stream.end.");
	end_stream_base += language;
	SString start_box = props->Get(start_box_base.c_str());
	SString start_stream = props->Get(start_stream_base.c_str());
	SString end_box = props->Get(end_box_base.c_str());
	SString end_stream = props->Get(end_stream_base.c_str());
	SString mark("-*-");
	int text_length = SendEditor(SCI_GETTEXTLENGTH);
	char buf[MAXBUF];
	int bufmax = text_length < MAXBUF ? text_length : MAXBUF;
	
	GetRange(wEditor, 0, bufmax - 1, buf);
	
	bool start_comment = false;
	bool indent_comment = false;
	int end_indent_comment = 0;
	
	for (int index = 0; index < bufmax; index++)
	{
		if (!start_comment)
		{
			if (memcmp(buf+index, start_box.c_str(), start_box.length()) == 0)
			{
				index += (start_box.length() - 1);
				start_comment = true;
				continue;
			}
			if (memcmp(buf+index, start_stream.c_str(), start_stream.length()) == 0)
			{
				index += (start_stream.length() - 1);
				start_comment = true;
				continue;
			}
			if (buf[index] != ' ' && buf[index] != '\t' && buf[index] != '\n')
				break;
		}
		else
		{
			if (!indent_comment)
			{
				if (buf[index] == ' ' || buf[index] == '\t' || buf[index] == '\n')
					continue;
				if (memcmp(buf+index, mark.c_str(), 3) == 0)
				{
					index += 3;
					indent_comment = true;
				}
				else
					break;
			}
			else
			{
				if (memcmp(buf+index, end_box.c_str(), end_box.length()) == 0)
				{
					end_indent_comment = index + end_box.length() - 1;
					break;
				}
				if (memcmp(buf+index, end_stream.c_str(), end_stream.length()) == 0)
				{
					end_indent_comment = index + end_stream.length() - 1;
					break;
				}
			}
		}		
	}
	SString mode = GetMode(language);
	if (mode.c_str() != "")
	{
		SString comment ;
		comment += start_stream.c_str() ;
		comment += " ";
		comment += mark.c_str();
		comment += mode.c_str();
		comment += mark.c_str();
		comment += " ";
		comment += end_stream.c_str() ;
	
		if (indent_comment)
		{
			SendEditor(SCI_SETSEL, 0, end_indent_comment + 1);
			SendEditorString(SCI_REPLACESEL, 0, comment.c_str());
		}
		else
		{
			comment += "\n\n";
			SendEditorString(SCI_INSERTTEXT, 0, comment.c_str());
		}
	}
	return TRUE;
}
	
#endif

/**
 * Return the length of the given line, not counting the EOL.
 */
int AnEditor::GetLineLength(int line) {
	return SendEditor(SCI_GETLINEENDPOSITION, line) - SendEditor(SCI_POSITIONFROMLINE, line);
}

int AnEditor::GetCurrentLineNumber() {
	CharacterRange crange = GetSelection();
	int selStart = crange.cpMin;
	return SendEditor(SCI_LINEFROMPOSITION, selStart);
}

int AnEditor::GetCurrentScrollPosition() {
	int lineDisplayTop = SendEditor(SCI_GETFIRSTVISIBLELINE);
	return SendEditor(SCI_DOCLINEFROMVISIBLE, lineDisplayTop);
}

// Upon a character being added, AnEditor may decide to perform some action
// such as displaying a completion list.
void AnEditor::CharAdded(char ch) {
	CharacterRange crange = GetSelection();
	int selStart = crange.cpMin;
	int selEnd = crange.cpMax;
	if ((selEnd == selStart) && (selStart > 0)) {
		int style = SendEditor(SCI_GETSTYLEAT, selStart - 1, 0);
		if (style != 1) {
			if (SendEditor(SCI_CALLTIPACTIVE)) {		// calltip is active
			} else if (SendEditor(SCI_AUTOCACTIVE)) {	// word autocompletion
			} else if (HandleXml(ch)) {
				// Handled in the routine
			} else {	// we don't have autocompetion nor calltip active
					if (indentMaintain)
						MaintainIndentation(ch);
			}
		}
	}
}

/**
 * This routine will auto complete XML or HTML tags that are still open by closing them
 * @parm ch The characer we are dealing with, currently only works with the '/' character
 * @return True if handled, false otherwise
 */
bool AnEditor::HandleXml(char ch) {
	// We're looking for this char
	// Quit quickly if not found
	if (ch != '>') {
		return false;
	}

	// This may make sense only in certain languages
	if (lexLanguage != SCLEX_HTML && lexLanguage != SCLEX_XML &&
	        lexLanguage != SCLEX_ASP && lexLanguage != SCLEX_PHP) {
		return false;
	}

	// If the user has turned us off, quit now.
	// Default is off
	SString value = props->GetExpanded("xml.auto.close.tags");
	if ((value.length() == 0) || (value == "0")) {
		return false;
	}

	// Grab the last 512 characters or so
	int nCaret = SendEditor(SCI_GETCURRENTPOS);
	char sel[512];
	int nMin = nCaret - (sizeof(sel) - 1);
	if (nMin < 0) {
		nMin = 0;
	}

	if (nCaret - nMin < 3) {
		return false; // Smallest tag is 3 characters ex. <p>
	}
	GetRange(wEditor, nMin, nCaret, sel);
	sel[sizeof(sel) - 1] = '\0';

	if (sel[nCaret - nMin - 2] == '/') {
		// User typed something like "<br/>"
		return false;
	}

	SString strFound = FindOpenXmlTag(sel, nCaret - nMin);

	if (strFound.length() > 0) {
		SendEditor(SCI_BEGINUNDOACTION);
		SString toInsert = "</";
		toInsert += strFound;
		toInsert += ">";
		SendEditorString(SCI_REPLACESEL, 0, toInsert.c_str());
		SetSelection(nCaret, nCaret);
		SendEditor(SCI_ENDUNDOACTION);
		return true;
	}

	return false;
}

/** Search backward through nSize bytes looking for a '<', then return the tag if any
 * @return The tag name
 */
SString AnEditor::FindOpenXmlTag(const char sel[], int nSize) {
	SString strRet = "";

	if (nSize < 3) {
		// Smallest tag is "<p>" which is 3 characters
		return strRet;
	}
	const char* pBegin = &sel[0];
	const char* pCur = &sel[nSize - 1];

	pCur--; // Skip past the >
	while (pCur > pBegin) {
		if (*pCur == '<') {
			break;
		} else if (*pCur == '>') {
			break;
		}
		--pCur;
	}

	if (*pCur == '<') {
		pCur++;
		while (strchr(":_-.", *pCur) || isalnum(*pCur)) {
			strRet += *pCur;
			pCur++;
		}
	}

	// Return the tag name or ""
	return strRet;
}

void AnEditor::GoMatchingBrace(bool select) {
	int braceAtCaret = -1;
	int braceOpposite = -1;
	bool isInside = FindMatchingBracePosition(true, braceAtCaret, braceOpposite, true);
	// Convert the character positions into caret positions based on whether
	// the caret position was inside or outside the braces.
	if (isInside) {
		if (braceOpposite > braceAtCaret) {
			braceAtCaret++;
		} else {
			braceOpposite++;
		}
	} else {    // Outside
		if (braceOpposite > braceAtCaret) {
			braceOpposite++;
		} else {
			braceAtCaret++;
		}
	}
	if (braceOpposite >= 0) {
		EnsureRangeVisible(braceOpposite, braceOpposite);
		if (select) {
			SetSelection(braceAtCaret, braceOpposite);
		} else {
			SetSelection(braceOpposite, braceOpposite);
		}
	}
}

int ControlIDOfCommand(unsigned long wParam) {
	return wParam & 0xffff;
}

long AnEditor::Command(int cmdID, long wParam, long lParam) {
	switch (cmdID) {
			
	case ANE_INSERTTEXT:
		SendEditor(SCI_INSERTTEXT,wParam,lParam);
		break;
	
	case ANE_GETBOOKMARK_POS:
		return GetBookmarkLine( wParam );
		break;	/* pleonastico */

	case ANE_BOOKMARK_TOGGLE_LINE:
		BookmarkToggle( wParam );
		break;

	case ANE_UNDO:
		SendEditor(SCI_UNDO);
		break;

	case ANE_REDO:
		SendEditor(SCI_REDO);
		break;

	case ANE_CUT:
		SendEditor(SCI_CUT);
		break;

	case ANE_COPY:
		SendEditor(SCI_COPY);
		break;

	case ANE_PASTE:
		SendEditor(SCI_PASTE);
		break;

	case ANE_CLEAR:
		SendEditor(SCI_CLEAR);
		break;

	case ANE_SELECTALL:
		SendEditor(SCI_SELECTALL);
		break;

	case ANE_FIND:
		return Find (wParam, (char*) lParam);

	case ANE_GOTOLINE:
		SendEditor(SCI_GOTOLINE, wParam);
		break;
	
	case ANE_SETZOOM:
		SendEditor(SCI_SETZOOM, wParam);
		break;

	case ANE_MATCHBRACE:
		GoMatchingBrace(false);
		break;

	case ANE_SELECTBLOCK:
		SelectBlock();
		break;

	case ANE_SELECTTOBRACE:
		GoMatchingBrace(true);
		break;
	
	case ANE_GETBLOCKSTARTLINE:
		return GetBlockStartLine();

	case ANE_GETBLOCKENDLINE:
		return GetBlockEndLine();
	
	case ANE_GETCURRENTWORD:
		return GetCurrentWord((char*)wParam, (int)lParam);
		
	case ANE_GETWORDBEFORECARAT:
		return GetWordBeforeCarat((char*)wParam, (int)lParam);

	case ANE_SHOWCALLTIP:
		StartCallTip_new();
		break;

	case ANE_COMPLETECALLTIP:
		CompleteCallTip();	
		break;
	/*
	case ANE_COMPLETE:
		StartAutoComplete();
		break;

	case ANE_COMPLETEWORD:
		StartAutoCompleteWord(false);
		break;
	*/
	case ANE_TOGGLE_FOLD:
		FoldToggle();
		break;

	case ANE_OPEN_FOLDALL:
		FoldOpenAll();
		break;

	case ANE_CLOSE_FOLDALL:
		FoldCloseAll();
		break;

	case ANE_UPRCASE:
		SendEditor(SCI_UPPERCASE);
		break;

	case ANE_LWRCASE:
		SendEditor(SCI_LOWERCASE);
		break;

	case ANE_EXPAND:
		SendEditor(SCI_TOGGLEFOLD, GetCurrentLineNumber());
		break;

	case ANE_LINENUMBERMARGIN:
		lineNumbers = wParam;
		SendEditor(SCI_SETMARGINWIDTHN, 0, lineNumbers ? lineNumbersWidth : 0);
		break;

	case ANE_SELMARGIN:
		margin = wParam;
		SendEditor(SCI_SETMARGINWIDTHN, 1, margin ? marginWidth : 0);
		break;

	case ANE_FOLDMARGIN:
		foldMargin = wParam;
		SendEditor(SCI_SETMARGINWIDTHN, 2, foldMargin ? foldMarginWidth : 0);
		break;

	case ANE_VIEWEOL:
		SendEditor(SCI_SETVIEWEOL, wParam);
		break;

	case ANE_EOL_CRLF:
		SendEditor(SCI_SETEOLMODE, SC_EOL_CRLF);
		break;

	case ANE_EOL_CR:
		SendEditor(SCI_SETEOLMODE, SC_EOL_CR);
		break;

	case ANE_EOL_LF:
		SendEditor(SCI_SETEOLMODE, SC_EOL_LF);
		break;

	case ANE_EOL_CONVERT:
		switch (wParam) {
			case ANE_EOL_CRLF:
				SendEditor(SCI_SETEOLMODE, SC_EOL_CRLF);
				SendEditor(SCI_CONVERTEOLS, SC_EOL_CRLF);
				break;
			case ANE_EOL_LF:
				SendEditor(SCI_SETEOLMODE, SC_EOL_LF);
				SendEditor(SCI_CONVERTEOLS, SC_EOL_LF);
				break;
			case ANE_EOL_CR:
				SendEditor(SCI_SETEOLMODE, SC_EOL_CR);
				SendEditor(SCI_CONVERTEOLS, SC_EOL_CR);
				break;
			default:
				SendEditor(SCI_CONVERTEOLS, SendEditor(SCI_GETEOLMODE));
				break;
		}
		break;

	case ANE_WORDPARTLEFT:
		SendEditor(SCI_WORDPARTLEFT);
		break;

	case ANE_WORDPARTLEFTEXTEND:
		SendEditor(SCI_WORDPARTLEFTEXTEND);
		break;

	case ANE_WORDPARTRIGHT:
		SendEditor(SCI_WORDPARTRIGHT);
		break;

	case ANE_WORDPARTRIGHTEXTEND:
		SendEditor(SCI_WORDPARTRIGHTEXTEND);
		break;
	
	case ANE_VIEWSPACE:
		ViewWhitespace(wParam);
		break;

	case ANE_VIEWGUIDES:
		SendEditor(SCI_SETINDENTATIONGUIDES, wParam);
		break;

	case ANE_BOOKMARK_TOGGLE:
		BookmarkToggle();
		break;
	case ANE_BOOKMARK_FIRST:
		BookmarkFirst();
		break;
	case ANE_BOOKMARK_PREV:
		BookmarkPrev();
		break;
	case ANE_BOOKMARK_NEXT:
		BookmarkNext();
		break;
	case ANE_BOOKMARK_LAST:
		BookmarkLast();
		break;
	case ANE_BOOKMARK_CLEAR:
		BookmarkClear();
		break;

	case ANE_SETTABSIZE:
		SendEditor(SCI_SETTABWIDTH, wParam);
		break;

	case ANE_SETLANGUAGE:
		SetOverrideLanguage(wParam);
		break;

	case ANE_SETHILITE:
		ReadProperties((char*)wParam);
		SendEditor(SCI_COLOURISE, 0, -1);
		break;

	case ANE_SETACCELGROUP:
		SetAccelGroup((GtkAccelGroup*)wParam);
		break;

	case ANE_GETTEXTRANGE: {
			guint start, end;
			if(wParam == lParam) return 0;
			start = (guint) MINIMUM(wParam, lParam);
			end = (guint) MAXIMUM(wParam, lParam);
			gchar *buff = (gchar*) g_malloc(end-start+10);
			if(!buff) return 0;
			GetRange(start, end, buff, false);
			return (long) buff;
		}
		break;

	case ANE_INDENT_INCREASE:
		IndentationIncrease();
		break;
	
	case ANE_INDENT_DECREASE:
		IndentationDecrease();
		break;
	
	case ANE_GETLENGTH:
		return SendEditor(SCI_GETLENGTH);

	case ANE_GET_LINENO:
		return GetCurrentLineNumber();
	
	case ANE_LINEWRAP:
		SetLineWrap((bool)wParam);
		break;
	
	case ANE_READONLY:
		SetReadOnly((bool)wParam);
		break;
	
	case ANE_GETSTYLEDTEXT: {
			guint start, end;
			if(wParam == lParam) return 0;
			start = (guint) MINIMUM(wParam, lParam);
			end = (guint) MAXIMUM(wParam, lParam);
			/* Allocate a bit more space to allow reading multi
			 * byte characters more easily */
			gchar *buff = (gchar*) g_malloc((end-start+10)*2);
			if(!buff) return 0;
			GetRange(start, end, buff, true);
			memset (buff + (end-start) * 2, 0, 20);
			return (long) buff;
		}
		break;
	case ANE_TEXTWIDTH:
		return SendEditor(SCI_TEXTWIDTH, wParam, lParam);
	case ANE_GETLANGUAGE:
		return (long) language.c_str();

	case ANE_BLOCKCOMMENT:
		return StartBlockComment();
	
	case ANE_BOXCOMMENT:
		return StartBoxComment();
	
	case ANE_STREAMCOMMENT:
		return StartStreamComment();
	/*
	case ANE_CUSTOMINDENT:
		return InsertCustomIndent();
	*/
	case ANE_WORDSELECT:
		WordSelect();
		break;
	
	case ANE_LINESELECT:
		LineSelect();
		break;
	
	case ANE_GETCURRENTPOS:
		return SendEditor(SCI_GETCURRENTPOS);
	
	case ANE_GOTOPOS:
		return SendEditor(SCI_GOTOPOS, wParam);
	
	case ANE_SETWRAPBOOKMARKS:
		// Nothing to do.
		break;
	/*
	case ANE_SETAUTOINDENTATION:
		// Nothing to do.
		break;
	*/
	case ANE_SETUSETABFORINDENT:
		SendEditor(SCI_SETUSETABS, wParam);
		break;
		
	case ANE_SETINDENTSIZE:
		indentSize = wParam;
		SendEditor(SCI_SETINDENT, wParam);
		break;
	
	case ANE_SETINDENTBRACESCHECK:
		bracesCheck = wParam;
		break;
	/*
	case ANE_SETINDENTOPENING:
		indentOpening = wParam;
		break;
		
	case ANE_SETINDENTCLOSING:
		indentClosing = wParam;
		break;
	*/
	case ANE_SETINDENTMAINTAIN:
		if (wParam)
			props->Set ("indent.maintain.*", "1");
		else
			props->Set ("indent.opening.*", "0");
		indentMaintain = wParam;
		break;
	
	case ANE_SETTABINDENTS:
		SendEditor(SCI_SETTABINDENTS, wParam);
		break;
		
	case ANE_SETBACKSPACEUNINDENTS:
		SendEditor(SCI_SETBACKSPACEUNINDENTS, wParam);
		break;
	
	case ANE_SETFOLDSYMBOLS:
		SetFoldSymbols(reinterpret_cast<char *> (wParam));
		break;
		
	case ANE_SETFOLDUNDERLINE:
		if (wParam)
			SendEditor(SCI_SETFOLDFLAGS, props->GetInt("fold.flags"));
		else
			SendEditor(SCI_SETFOLDFLAGS, 0);
		break;
	case ANE_SETLINENUMWIDTH:
		lineNumbersWidth = wParam;
		SendEditor(SCI_SETMARGINWIDTHN, 0, lineNumbers ? lineNumbersWidth : 0);
		break;
	case ANE_SETEDGECOLUMN:
		SendEditor(SCI_SETEDGECOLUMN, wParam);
		break;
	default:
		break;
	}
	return 0;
}

void AnEditor::FoldChanged(int line, int levelNow, int levelPrev) {
	if (levelNow & SC_FOLDLEVELHEADERFLAG) {
		SendEditor(SCI_SETFOLDEXPANDED, line, 1);
	} else if (levelPrev & SC_FOLDLEVELHEADERFLAG) {
		if (!SendEditor(SCI_GETFOLDEXPANDED, line)) {
			// Removing the fold from one that has been contracted so should expand
			// otherwise lines are left invisible with no way to make them visible
			Expand(line, true, false, 0, levelPrev);
		}
	}
}

void AnEditor::Expand(int &line, bool doExpand, bool force, int visLevels, int level) {
	int lineMaxSubord = SendEditor(SCI_GETLASTCHILD, line, level);
	line++;
	while (line <= lineMaxSubord) {
		if (force) {
			if (visLevels > 0)
				SendEditor(SCI_SHOWLINES, line, line);
			else
				SendEditor(SCI_HIDELINES, line, line);
		} else {
			if (doExpand)
				SendEditor(SCI_SHOWLINES, line, line);
		}
		int levelLine = level;
		if (levelLine ==-1)
			levelLine = SendEditor(SCI_GETFOLDLEVEL, line);
		if (levelLine & SC_FOLDLEVELHEADERFLAG) {
			if (force) {
				if (visLevels > 1)
					SendEditor(SCI_SETFOLDEXPANDED, line, 1);
				else
					SendEditor(SCI_SETFOLDEXPANDED, line, 0);
				Expand(line, doExpand, force, visLevels - 1);
			} else {
				if (doExpand && SendEditor(SCI_GETFOLDEXPANDED, line)) {
					Expand(line, true, force, visLevels - 1);
				} else {
					Expand(line, false, force, visLevels - 1);
				}
			}
		} else {
			line++;
		}
	}
}

void AnEditor::FoldCode(bool expanding) {
	int maxLine = SendEditor (SCI_GETTEXTLENGTH);
	SendEditor(SCI_COLOURISE, 0, -1);
	for (int line = 0; line < maxLine; line++) {
		int level = SendEditor(SCI_GETFOLDLEVEL, line);
		if ((level & SC_FOLDLEVELHEADERFLAG) &&
		        (SC_FOLDLEVELBASE == (level & SC_FOLDLEVELNUMBERMASK))) {
			if (expanding) {
				SendEditor(SCI_SETFOLDEXPANDED, line, 1);
				Expand(line, true);
				line--;
			} else {
				int lineMaxSubord = SendEditor(SCI_GETLASTCHILD, line, -1);
				SendEditor(SCI_SETFOLDEXPANDED, line, 0);
				if (lineMaxSubord > line)
					SendEditor(SCI_HIDELINES, line + 1, lineMaxSubord);
			}
		}
	}
}

void AnEditor::FoldOpenAll() {
	FoldCode (true);
}

void AnEditor::FoldCloseAll() {
	FoldCode (false);
}

void AnEditor::FoldToggle() {
	int curLine = SendEditor(SCI_LINEFROMPOSITION, SendEditor (SCI_GETCURRENTPOS));
	int level = SendEditor(SCI_GETFOLDLEVEL, curLine);
	if (level & SC_FOLDLEVELHEADERFLAG) {
		SendEditor(SCI_TOGGLEFOLD, curLine);
		return;
	}
	int parent = SendEditor (SCI_GETFOLDPARENT, curLine);
	int lastChild = SendEditor (SCI_GETLASTCHILD, parent, -1);
	if (curLine > parent && curLine <= lastChild)
	{
		SendEditor(SCI_TOGGLEFOLD, parent);
		SendEditor(SCI_SETCURRENTPOS, SendEditor (SCI_POSITIONFROMLINE, parent));
		SendEditor(SCI_GOTOLINE, parent);
	}
	else
		gdk_beep ();
}

void AnEditor::SelectBlock () {
	int curLine = SendEditor(SCI_LINEFROMPOSITION, SendEditor (SCI_GETCURRENTPOS));
	int parent = SendEditor (SCI_GETFOLDPARENT, curLine);
	int lastChild = SendEditor (SCI_GETLASTCHILD, parent, -1);
	if (curLine > parent && curLine <= lastChild)
	{
		gint start, end;
		start = SendEditor(SCI_POSITIONFROMLINE, parent);
		end = SendEditor(SCI_POSITIONFROMLINE, lastChild+1);
		SetSelection(start, end);
	}
	else
		gdk_beep ();
}

int AnEditor::GetBlockStartLine (int curLine) {
	if(curLine < 0)
	{
	  curLine = SendEditor(SCI_LINEFROMPOSITION, SendEditor (SCI_GETCURRENTPOS));
	}
	int level = SendEditor(SCI_GETFOLDLEVEL, curLine);
	if (level & SC_FOLDLEVELHEADERFLAG) {
		return curLine;
	}
	int parent = -1;
	int lastChild = curLine;
	while(parent == -1)
	{
		parent = SendEditor (SCI_GETFOLDPARENT, lastChild);
		if (parent == -1)
		{
			return -1;
		}
		lastChild = SendEditor (SCI_GETLASTCHILD, parent, -1);
		if (curLine > parent && curLine <= lastChild)
	  {
		  return parent;
	  }
	  lastChild = parent - 1;
		parent = -1;
	}
	return -1;
}

int AnEditor::GetBlockEndLine (int curLine) {
	if(curLine < 0)
	{
	  curLine = SendEditor(SCI_LINEFROMPOSITION, SendEditor (SCI_GETCURRENTPOS));
	}
	int level = SendEditor(SCI_GETFOLDLEVEL, curLine);
	if (level & SC_FOLDLEVELHEADERFLAG) {
		/* this may be a problem when we use this function on "start" block line */
		return curLine;
	}
  int parent = -1;
  int lastChild = curLine;
	while(parent == -1)
	{
		parent = SendEditor (SCI_GETFOLDPARENT, lastChild);
		if (parent == -1)
		{
			return -1;
		}
		lastChild = SendEditor (SCI_GETLASTCHILD, parent, -1);
		if (curLine > parent && curLine <= lastChild)
	  {
		  return lastChild;
	  }
	  lastChild = parent - 1;
		parent = -1;
	}
	return -1;
}

void AnEditor::EnsureRangeVisible(int posStart, int posEnd) {
	int lineStart = SendEditor(SCI_LINEFROMPOSITION, Platform::Minimum(posStart, posEnd));
	int lineEnd = SendEditor(SCI_LINEFROMPOSITION, Platform::Maximum(posStart, posEnd));
	for (int line = lineStart; line <= lineEnd; line++) {
		SendEditor(SCI_ENSUREVISIBLE, line);
	}
}

void AnEditor::SetLineWrap(bool wrap) {
	wrapLine = wrap;
	SendEditor(SCI_SETWRAPMODE, wrapLine ? SC_WRAP_WORD : SC_WRAP_NONE);
	SendEditor(SCI_SETHSCROLLBAR, !wrapLine);
}

void AnEditor::SetReadOnly(bool readonly) {
	isReadOnly = readonly;
	SendEditor(SCI_SETREADONLY, isReadOnly);
}

bool AnEditor::MarginClick(int position, int modifiers) {
	int lineClick = SendEditor(SCI_LINEFROMPOSITION, position);
	//	SendEditor(SCI_GETFOLDLEVEL, lineClick) & SC_FOLDLEVELHEADERFLAG);
	if (modifiers & SCMOD_SHIFT) {
		FoldCloseAll();
	} else if (modifiers & SCMOD_CTRL) {
		FoldOpenAll();
	} else if (SendEditor(SCI_GETFOLDLEVEL, lineClick) & SC_FOLDLEVELHEADERFLAG) {
		if (modifiers & SCMOD_SHIFT) {
			// Ensure all children visible
			SendEditor(SCI_SETFOLDEXPANDED, lineClick, 1);
			Expand(lineClick, true, true, 100);
		} else if (modifiers & SCMOD_CTRL) {
			if (SendEditor(SCI_GETFOLDEXPANDED, lineClick)) {
				// Contract this line and all children
				SendEditor(SCI_SETFOLDEXPANDED, lineClick, 0);
				Expand(lineClick, false, true, 0);
			} else {
				// Expand this line and all children
				SendEditor(SCI_SETFOLDEXPANDED, lineClick, 1);
				Expand(lineClick, true, true, 100);
			}
		} else {
			// Toggle this line
			SendEditor(SCI_TOGGLEFOLD, lineClick);
		}
	}
	return true;
}

#if 0
gint AnEditor::KeyPressEvent(GtkWidget *, GdkEventKey *event, AnEditor *anedit) {
	return anedit->KeyPress(event->state, event->keyval);
}
#endif

void AnEditor::NotifySignal(GtkWidget *, gint /*wParam*/, gpointer lParam, AnEditor *anedit) {
	anedit->Notify(reinterpret_cast<SCNotification *>(lParam));
}

// FIXME:
#if 0
void
eval_output_arrived_for_aneditor(GList* lines, gpointer data)
{
	// We expect lines->data to be a string of the form VARIABLE = VALUE,
	// and 'data' to be a pointer to an object of type
	// 'ExpressionEvaluationTipInfo'.

	if (data == NULL)
		return;
	
	auto_ptr<ExpressionEvaluationTipInfo> info(
			(ExpressionEvaluationTipInfo *) data);

	if (info->editor == NULL)
	    return;

	if (info->editor != aneditor_get(AnEditor::focusedID))
		return;

	info->editor->EvalOutputArrived(lines, info->textPos, info->expression);
}

void AnEditor::EvalOutputArrived(GList* lines, int textPos,
								 const string &expression) {
	
	if (textPos <= 0)
	    return;

	// Return if debug Tip has been canceled
	if (!debugTipOn)
		return;

	if (g_list_length(lines) == 0 || lines->data == NULL)
	    return;

	string result = (char *) lines->data;
	string::size_type posEquals = result.find(" = ");
	if (posEquals != string::npos)
	    result.replace(0, posEquals, expression);

	SendEditorString(SCI_CALLTIPSHOW, textPos, result.c_str());
	SendEditor(SCI_CALLTIPSETHLT, 0, result.length());
}

void AnEditor::EndDebugEval() {
	if (debugTipOn)
	{
		SendEditor(SCI_CALLTIPCANCEL);
		debugTipOn = false;
	}
}

void AnEditor::HandleDwellStart(int mousePos) {
	if (mousePos == -1)
		return;

	char expr[256];
	if (!debugger_is_active() || !debugger_is_ready())
	{
		// Do not show expression tip if it can't be shown.
		// string s = string(expr) + ": " + _("debugger not active");
		// SendEditorString(SCI_CALLTIPSHOW, mousePos, s.c_str());
		return;
	}
	// If debug tip is already running, return.
	if (debugTipOn)
		return;

	CharacterRange crange = GetSelection();
	if (crange.cpMin == crange.cpMax
			|| mousePos < crange.cpMin
			|| mousePos >= crange.cpMax)
	{
		// There is no selection, or the mouse pointer is
		// out of the selection, so we search for a word
		// around the mouse pointer:
		if (!GetWordAtPosition(expr, sizeof(expr), mousePos))
			return;
	}
	else
	{
		long lensel = crange.cpMax - crange.cpMin;
		long max = sizeof(expr) - 1;
		guint end = (lensel < max ? crange.cpMax : crange.cpMin + max);
		GetRange(crange.cpMin, end, expr, false);

		// If there is any control character except TAB
		// in the expression, disregard it.
		size_t i;
		for (i = 0; i < end - crange.cpMin; i++)
			if ((unsigned char) expr[i] < ' ' && expr[i] != '\t')
				return;
		if (i < end - crange.cpMin)
		    return;
	}

	// Imitation of on_eval_ok_clicked():
	// The function eval_output_arrived_for_aneditor() will
	// be called eventually by the debugger with the result
	// of the print command for 'expr', and with the 'info'
	// pointer.  That function must call delete on 'info'.
	//
	// We don't turn GDB "pretty printing" on because we want
	// the entire value on a single line, in the case of a
	// struct or class.
	// We don't want static members of classes to clutter up
	// the displayed tip, however.

	ExpressionEvaluationTipInfo *info =
			new ExpressionEvaluationTipInfo(this, mousePos, expr);
	debugger_query_evaluate_expr_tip (expr, eval_output_arrived_for_aneditor, info);
	debugger_query_execute ();
	debugTipOn = true;
}
#endif

#if 0
int AnEditor::KeyPress(unsigned int state, unsigned int keyval){

	unsigned int mask = GDK_SHIFT_MASK | GDK_LOCK_MASK |
		GDK_CONTROL_MASK | GDK_MOD1_MASK | GDK_MOD3_MASK |
		GDK_MOD4_MASK |	GDK_MOD5_MASK;
	if (state & mask)
		return false;

	// Trap 'TAB' key for automatic indentation.
	// printf ("Key is '%c'\n", notification->ch);
	if ((keyval == GDK_Tab) &&
		(lexLanguage == SCLEX_CPP) &&
		(!indentMaintain) &&
		(props->GetInt("indent.automatic")) &&
		(!SendEditor(SCI_CALLTIPACTIVE)) &&
		(!SendEditor(SCI_AUTOCACTIVE))) {
			
		CharacterRange crange = GetSelection();
		int selStart = crange.cpMin;
		int selEnd = crange.cpMax;
		
		if (selStart == selEnd) {
			AutomaticIndentation('\t');
			return true;
		}
	}
	return false;
}
#endif

void AnEditor::Notify(SCNotification *notification) {
	switch (notification->nmhdr.code) {
	case SCN_CALLTIPCLICK:
			if (notification->position == 1) {
				call_tip_node.def_index--;
				if (call_tip_node.def_index < 0)
					call_tip_node.def_index = 0;
			}
			if (notification->position == 2) {
				call_tip_node.def_index++;
				if (call_tip_node.def_index >= call_tip_node.max_def)
					call_tip_node.def_index = call_tip_node.max_def - 1;
			}
			ResumeCallTip (false);
			break;
	case SCN_KEY: {
		if(!accelGroup) break;
		int mods = 0;
		if (notification->modifiers & SCMOD_SHIFT)
			mods |= GDK_SHIFT_MASK;
		if (notification->modifiers & SCMOD_CTRL)
			mods |= GDK_CONTROL_MASK;
		if (notification->modifiers & SCMOD_ALT)
			mods |= GDK_MOD1_MASK;
		gtk_accel_groups_activate(G_OBJECT (accelGroup), notification->ch,
			static_cast<GdkModifierType>(mods));
	}

	case SCN_CHARADDED:
		CharAdded(static_cast<char>(notification->ch));
		break;
	
	case SCN_SAVEPOINTREACHED:
		isDirty = false;
		break;

	case SCN_SAVEPOINTLEFT:
		isDirty = true;
		break;

	case SCN_UPDATEUI:
	{
		int pos = SendEditor(SCI_GETCURRENTPOS);
		BraceMatch(true);
		if (SendEditor(SCI_CALLTIPACTIVE) ) {
			// if we have a caret movement on left or right
			if (abs(pos - lastPos) == 1 ) {
				ContinueCallTip_new();
			}
		}
		lastPos = pos;
	}
		break;

	case SCN_MODIFIED:
		if (notification->modificationType == SC_MOD_CHANGEFOLD) {
			FoldChanged(notification->line,
			            notification->foldLevelNow, notification->foldLevelPrev);
		}
		break;

	case SCN_MARGINCLICK:
		if (notification->margin == 2)
			MarginClick(notification->position, notification->modifiers);
		break;

	case SCN_NEEDSHOWN: {
			EnsureRangeVisible(notification->position, notification->position + notification->length);
		}
		break;
/*
	case SCN_DWELLSTART:
		HandleDwellStart(notification->position);
		break;

	case SCN_DWELLEND:
		EndDebugEval();
		// SendEditor(SCI_CALLTIPCANCEL);
		break;
*/
	}
}

static int IntFromHexDigit(const char ch) {
	if (isdigit(ch))
		return ch - '0';
	else if (ch >= 'A' && ch <= 'F')
		return ch - 'A' + 10;
	else if (ch >= 'a' && ch <= 'f')
		return ch - 'a' + 10;
	else
		return 0;
}

static Colour ColourFromString(const char *val) {
	int r = IntFromHexDigit(val[1]) * 16 + IntFromHexDigit(val[2]);
	int g = IntFromHexDigit(val[3]) * 16 + IntFromHexDigit(val[4]);
	int b = IntFromHexDigit(val[5]) * 16 + IntFromHexDigit(val[6]);
	return Colour(r, g, b);
}

static long ColourOfProperty(PropSet *props, const char *key, ColourDesired colourDefault) {
	SString colour = props->Get(key);
	if (colour.length()) {
		return ColourFromString(colour.c_str()).AsLong();
	}
	return colourDefault.AsLong();
}

void AnEditor::SetOneStyle(Window &win, int style, const char *s) {
	char *val = StringDup(s);
	char *opt = val;
	while (opt) {
		char *cpComma = strchr(opt, ',');
		if (cpComma)
			*cpComma = '\0';
		char *colon = strchr(opt, ':');
		if (colon)
			*colon++ = '\0';
		if (0 == strcmp(opt, "italics"))
			Platform::SendScintilla(win.GetID(), SCI_STYLESETITALIC, style, 1);
		if (0 == strcmp(opt, "notitalics"))
			Platform::SendScintilla(win.GetID(), SCI_STYLESETITALIC, style, 0);
		if (0 == strcmp(opt, "bold"))
			Platform::SendScintilla(win.GetID(), SCI_STYLESETBOLD, style, 1);
		if (0 == strcmp(opt, "notbold"))
			Platform::SendScintilla(win.GetID(), SCI_STYLESETBOLD, style, 0);
		if (0 == strcmp(opt, "font"))
			Platform::SendScintilla(win.GetID(), SCI_STYLESETFONT, style, reinterpret_cast<long>(colon));
		if (0 == strcmp(opt, "fore"))
			Platform::SendScintilla(win.GetID(), SCI_STYLESETFORE, style, ColourFromString(colon).AsLong());
		if (0 == strcmp(opt, "back"))
			Platform::SendScintilla(win.GetID(), SCI_STYLESETBACK, style, ColourFromString(colon).AsLong());
		if (0 == strcmp(opt, "size"))
			Platform::SendScintilla(win.GetID(), SCI_STYLESETSIZE, style, atoi(colon));
		if (0 == strcmp(opt, "eolfilled"))
			Platform::SendScintilla(win.GetID(), SCI_STYLESETEOLFILLED, style, 1);
		if (0 == strcmp(opt, "noteolfilled"))
			Platform::SendScintilla(win.GetID(), SCI_STYLESETEOLFILLED, style, 0);
		if (0 == strcmp(opt, "underlined"))
			Platform::SendScintilla(win.GetID(), SCI_STYLESETUNDERLINE, style, 1);
		if (0 == strcmp(opt, "notunderlined"))
			Platform::SendScintilla(win.GetID(), SCI_STYLESETUNDERLINE, style, 0);
		if (0 == strcmp(opt, "case")) {
			if (*colon == 'u') {
			Platform::SendScintilla(win.GetID(), SCI_STYLESETCASE, style, SC_CASE_UPPER);
			} else if (*colon == 'l') {
			Platform::SendScintilla(win.GetID(), SCI_STYLESETCASE, style, SC_CASE_LOWER);
			} else {
			Platform::SendScintilla(win.GetID(), SCI_STYLESETCASE, style, SC_CASE_MIXED);
			}
		}
		if (0 == strcmp(opt, "visible"))
			Platform::SendScintilla(win.GetID(), SCI_STYLESETVISIBLE, style, 1);
		if (0 == strcmp(opt, "notvisible"))
			Platform::SendScintilla(win.GetID(), SCI_STYLESETVISIBLE, style, 0);
		if (0 == strcmp(opt, "changeable"))
			Platform::SendScintilla(win.GetID(), SCI_STYLESETCHANGEABLE, style, 1);
		if (0 == strcmp(opt, "notchangeable"))
			Platform::SendScintilla(win.GetID(), SCI_STYLESETCHANGEABLE, style, 0);
		if (cpComma)
			opt = cpComma + 1;
		else
			opt = 0;
	}
	if (val)
		delete []val;
	Platform::SendScintilla(win.GetID(), SCI_STYLESETCHARACTERSET, style, characterSet);
}

void AnEditor::SetStyleFor(Window &win, const char *lang) {
	for (int style = 0; style <= STYLE_MAX; style++) {
		if (style != STYLE_DEFAULT) {
			char key[200];
			sprintf(key, "style.%s.%0d", lang, style);
			SString sval = props->GetExpanded(key);
			// g_print ("Style for %s:%0d == %s\n", lang, style, sval.c_str());
			SetOneStyle(win, style, sval.c_str());
		}
	}
}

void lowerCaseString(char *s) {
	while (*s) {
		*s = static_cast<char>(tolower(*s));
		s++;
	}
}

SString AnEditor::ExtensionFileName() {
	if (overrideExtension.length())
		return overrideExtension;
	else if (fileName[0]) {
		// Force extension to lower case
		char fileNameWithLowerCaseExtension[MAX_PATH];
		strcpy(fileNameWithLowerCaseExtension, fileName);
		char *extension = strrchr(fileNameWithLowerCaseExtension, '.');
		if (extension) {
			lowerCaseString(extension);
		}
		return SString(fileNameWithLowerCaseExtension);
	} else
		return props->Get("default.file.ext");
}

void AnEditor::ForwardPropertyToEditor(const char *key) {
	SString value = props->Get(key);
	SendEditorString(SCI_SETPROPERTY,
	                 reinterpret_cast<uptr_t>(key), value.c_str());
}

SString AnEditor::FindLanguageProperty(const char *pattern, const char *defaultValue) {
	SString key = pattern;
	key.substitute("*", language.c_str());
	SString ret = props->GetExpanded(key.c_str());
	if (ret == "")
		ret = props->GetExpanded(pattern);
	if (ret == "")
		ret = defaultValue;
	return ret;
}

void AnEditor::ReadProperties(const char *fileForExt) {
	//DWORD dwStart = timeGetTime();
	if (fileForExt)
		strcpy (fileName, fileForExt);
	else
		fileName[0] = '\0';
	
	SString fileNameForExtension;
	if(overrideExtension.length())
		fileNameForExtension = overrideExtension;
	else {
		fileNameForExtension = fileForExt;
	}

	language = props->GetNewExpand("lexer.", fileNameForExtension.c_str());
	SendEditorString(SCI_SETLEXERLANGUAGE, 0, language.c_str());
	lexLanguage = SendEditor(SCI_GETLEXER);

	if ((lexLanguage == SCLEX_HTML) || (lexLanguage == SCLEX_XML))
		SendEditor(SCI_SETSTYLEBITS, 7);
	else
		SendEditor(SCI_SETSTYLEBITS, 5);

	SendEditor(SCI_SETLEXER, lexLanguage);
	
	SString kw0 = props->GetNewExpand("keywords.", fileNameForExtension.c_str());
	SendEditorString(SCI_SETKEYWORDS, 0, kw0.c_str());
	SString kw2 = props->GetNewExpand("keywords3.", fileNameForExtension.c_str());
	SendEditorString(SCI_SETKEYWORDS, 2, kw2.c_str());
	/* For C/C++ projects, get list of typedefs for colorizing */
	/* TODO: Either remove or port to IAnjutaSymbolManager */
#if 0
	if (SCLEX_CPP == lexLanguage)
	{
		const TMWorkspace *workspace = tm_get_workspace();

		/* Assign global keywords */
		if ((workspace) && (workspace->global_tags))
		{
			GPtrArray *g_typedefs = tm_tags_extract(workspace->global_tags
			  , tm_tag_typedef_t | tm_tag_struct_t | tm_tag_class_t);
			if ((g_typedefs) && (g_typedefs->len > 0))
			{
				GString *s = g_string_sized_new(g_typedefs->len * 10);
				for (guint i = 0; i < g_typedefs->len; ++i)
				{
					if (!(TM_TAG(g_typedefs->pdata[i])->atts.entry.scope))
					{
						g_string_append(s, TM_TAG(g_typedefs->pdata[i])->name);
						g_string_append_c(s, ' ');
					}
				}
				SendEditorString(SCI_SETKEYWORDS, 3, s->str);
				g_string_free(s, TRUE);
			}
			g_ptr_array_free(g_typedefs, TRUE);
		}

		/* Assign project keywords */
		if ((workspace) && (workspace->work_object.tags_array))
		{
			GPtrArray *typedefs = tm_tags_extract(workspace->work_object.tags_array
			  , tm_tag_typedef_t | tm_tag_struct_t | tm_tag_class_t);
			if ((typedefs) && (typedefs->len > 0))
			{
				GString *s = g_string_sized_new(typedefs->len * 10);
				for (guint i = 0; i < typedefs->len; ++i)
				{
					if (!(TM_TAG(typedefs->pdata[i])->atts.entry.scope))
					{
						if (TM_TAG(typedefs->pdata[i])->name)
						{
						g_string_append(s, TM_TAG(typedefs->pdata[i])->name);
						g_string_append_c(s, ' ');
						}
					}
				}
				SendEditorString(SCI_SETKEYWORDS, 1, s->str);
				g_string_free(s, TRUE);
			}
			g_ptr_array_free(typedefs, TRUE);
		}
	}
	else
#endif
	{
		SString kw1 = props->GetNewExpand("keywords2.", fileNameForExtension.c_str());
		SendEditorString(SCI_SETKEYWORDS, 1, kw1.c_str());
		SString kw3 = props->GetNewExpand("keywords4.", fileNameForExtension.c_str());
		SendEditorString(SCI_SETKEYWORDS, 3, kw3.c_str());
		SString kw4 = props->GetNewExpand("keywords5.", fileNameForExtension.c_str());
		SendEditorString(SCI_SETKEYWORDS, 4, kw4.c_str());
		SString kw5 = props->GetNewExpand("keywords6.", fileNameForExtension.c_str());
		SendEditorString(SCI_SETKEYWORDS, 5, kw5.c_str());
	}

	ForwardPropertyToEditor("fold");
	ForwardPropertyToEditor("fold.use.plus");
	ForwardPropertyToEditor("fold.comment");
	ForwardPropertyToEditor("fold.comment.python");
	ForwardPropertyToEditor("fold.compact");
	ForwardPropertyToEditor("fold.html");
	ForwardPropertyToEditor("fold.preprocessor");
	ForwardPropertyToEditor("fold.quotes.python");
	ForwardPropertyToEditor("styling.within.preprocessor");
	ForwardPropertyToEditor("tab.timmy.whinge.level");
	ForwardPropertyToEditor("asp.default.language");
	
	// codePage = props->GetInt("code.page");
	//if (unicodeMode != uni8Bit) {
		// Override properties file to ensure Unicode displayed.
		// codePage = SC_CP_UTF8;
	// }
	// SendEditor(SCI_SETCODEPAGE, codePage);
	
	// Use unicode everytime.
	SendEditor(SCI_SETCODEPAGE, SC_CP_UTF8);
	
	characterSet = props->GetInt("character.set");
	setlocale(LC_CTYPE, props->Get("LC_CTYPE").c_str());

	SendEditor(SCI_SETCARETFORE,
	           ColourOfProperty(props, "caret.fore", ColourDesired(0, 0, 0)));
	SendEditor(SCI_SETCARETWIDTH, props->GetInt("caret.width", 1));
	SendEditor(SCI_SETMOUSEDWELLTIME, props->GetInt("dwell.period", 750), 0);
	
	SString caretLineBack = props->Get("caret.line.back");
	if (caretLineBack.length()) {
		SendEditor(SCI_SETCARETLINEVISIBLE, 1);
		SendEditor(SCI_SETCARETLINEBACK,
		           ColourFromString(caretLineBack.c_str()).AsLong());
	} else {
		SendEditor(SCI_SETCARETLINEVISIBLE, 0);
	}
	
	SString controlCharSymbol = props->Get("control.char.symbol");
	if (controlCharSymbol.length()) {
		SendEditor(SCI_SETCONTROLCHARSYMBOL, static_cast<unsigned char>(controlCharSymbol[0]));
	} else {
		SendEditor(SCI_SETCONTROLCHARSYMBOL, 0);
	}
	
	SendEditor(SCI_CALLTIPSETBACK,
	           ColourOfProperty(props, "calltip.back", ColourDesired(0xff, 0xff, 0xff)));
	
	SString caretPeriod = props->Get("caret.period");
	if (caretPeriod.length()) {
		SendEditor(SCI_SETCARETPERIOD, caretPeriod.value());
	}

	int caretSlop = props->GetInt("caret.policy.xslop", 1) ? CARET_SLOP : 0;
	int caretZone = props->GetInt("caret.policy.width", 50);
	int caretStrict = props->GetInt("caret.policy.xstrict") ? CARET_STRICT : 0;
	int caretEven = props->GetInt("caret.policy.xeven", 1) ? CARET_EVEN : 0;
	int caretJumps = props->GetInt("caret.policy.xjumps") ? CARET_JUMPS : 0;
	SendEditor(SCI_SETXCARETPOLICY, caretStrict | caretSlop | caretEven | caretJumps, caretZone);

	caretSlop = props->GetInt("caret.policy.yslop", 1) ? CARET_SLOP : 0;
	caretZone = props->GetInt("caret.policy.lines");
	caretStrict = props->GetInt("caret.policy.ystrict") ? CARET_STRICT : 0;
	caretEven = props->GetInt("caret.policy.yeven", 1) ? CARET_EVEN : 0;
	caretJumps = props->GetInt("caret.policy.yjumps") ? CARET_JUMPS : 0;
	SendEditor(SCI_SETYCARETPOLICY, caretStrict | caretSlop | caretEven | caretJumps, caretZone);

	int visibleStrict = props->GetInt("visible.policy.strict") ? VISIBLE_STRICT : 0;
	int visibleSlop = props->GetInt("visible.policy.slop", 1) ? VISIBLE_SLOP : 0;
	int visibleLines = props->GetInt("visible.policy.lines");
	SendEditor(SCI_SETVISIBLEPOLICY, visibleStrict | visibleSlop, visibleLines);

	SendEditor(SCI_SETEDGECOLUMN, props->GetInt("edge.column", 0));
	SendEditor(SCI_SETEDGEMODE, props->GetInt("edge.mode", EDGE_NONE));
	SendEditor(SCI_SETEDGECOLOUR,
	           ColourOfProperty(props, "edge.colour", ColourDesired(0xff, 0xda, 0xda)));

	SString selfore = props->Get("selection.fore");
	if (selfore.length()) {
		SendEditor(SCI_SETSELFORE, 1, ColourFromString(selfore.c_str()).AsLong());
	} else {
		SendEditor(SCI_SETSELFORE, 0, 0);
	}
	SString selBack = props->Get("selection.back");
	if (selBack.length()) {
		SendEditor(SCI_SETSELBACK, 1, ColourFromString(selBack.c_str()).AsLong());
	} else {
		if (selfore.length())
			SendEditor(SCI_SETSELBACK, 0, 0);
		else	// Have to show selection somehow
			SendEditor(SCI_SETSELBACK, 1, ColourDesired(0xC0, 0xC0, 0xC0).AsLong());
	}

	SString whitespaceFore = props->Get("whitespace.fore");
	if (whitespaceFore.length()) {
		SendEditor(SCI_SETWHITESPACEFORE, 1, ColourFromString(whitespaceFore.c_str()).AsLong());
	} else {
		SendEditor(SCI_SETWHITESPACEFORE, 0, 0);
	}
	SString whitespaceBack = props->Get("whitespace.back");
	if (whitespaceBack.length()) {
		SendEditor(SCI_SETWHITESPACEBACK, 1, ColourFromString(whitespaceBack.c_str()).AsLong());
	} else {
		SendEditor(SCI_SETWHITESPACEBACK, 0, 0);
	}

	for (int i = 0; i < 3; i++) {
		
		SString value_str;
		long default_indic_type[] = {INDIC_TT, INDIC_DIAGONAL, INDIC_SQUIGGLE};
		const char *default_indic_color[] = {"0000FF", "#00FF00", "#FF0000"};
		const char *style_name[] = {"normal", "warning", "error"};
		char key[200];
		sprintf(key, "indicators.style.%s", style_name[i]);

		value_str = props->Get(key);
		if (value_str.length() > 0) {
			if (strcasecmp (value_str.c_str(), "underline-plain") == 0) {
				SendEditor(SCI_INDICSETSTYLE, i, INDIC_PLAIN);
			} else if (strcasecmp (value_str.c_str(), "underline-tt") == 0) {
				SendEditor(SCI_INDICSETSTYLE, i, INDIC_TT);
			} else if (strcasecmp (value_str.c_str(), "underline-squiggle") == 0) {
				SendEditor(SCI_INDICSETSTYLE, i, INDIC_SQUIGGLE);
			} else if (strcasecmp (value_str.c_str(), "strike-out") == 0) {
				SendEditor(SCI_INDICSETSTYLE, i, INDIC_STRIKE);
			} else if (strcasecmp (value_str.c_str(), "diagonal") == 0) {
				SendEditor(SCI_INDICSETSTYLE, i, INDIC_DIAGONAL);
			} else {
				SendEditor(SCI_INDICSETSTYLE, i, default_indic_type[i]);
			}
		} else {
			SendEditor(SCI_INDICSETSTYLE, i, default_indic_type[i]);
		}
		sprintf(key, "indicator.%d.color", i);
		value_str = props->GetExpanded(key);
		if (value_str.length()) {
			SendEditor(SCI_INDICSETFORE, i, ColourFromString(value_str.c_str()).AsLong());
		} else {
			SendEditor(SCI_INDICSETFORE, i, ColourFromString(default_indic_color[i]).AsLong());
		}
	}
	
	char bracesStyleKey[200];
	sprintf(bracesStyleKey, "braces.%s.style", language.c_str());
	bracesStyle = props->GetInt(bracesStyleKey, 0);

	char key[200];
	SString sval;

	sval = FindLanguageProperty("calltip.*.ignorecase");
	callTipIgnoreCase = sval == "1";

	calltipWordCharacters = FindLanguageProperty("calltip.*.word.characters",
		"_abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ");

	calltipEndDefinition = FindLanguageProperty("calltip.*.end.definition");
	
	sprintf(key, "autocomplete.%s.start.characters", language.c_str());
	autoCompleteStartCharacters = props->GetExpanded(key);
	if (autoCompleteStartCharacters == "")
		autoCompleteStartCharacters = props->GetExpanded("autocomplete.*.start.characters");
	// "" is a quite reasonable value for this setting

	sprintf(key, "autocomplete.%s.fillups", language.c_str());
	autoCompleteFillUpCharacters = props->GetExpanded(key);
	if (autoCompleteFillUpCharacters == "")
		autoCompleteFillUpCharacters =
			props->GetExpanded("autocomplete.*.fillups");
	SendEditorString(SCI_AUTOCSETFILLUPS, 0,
		autoCompleteFillUpCharacters.c_str());

	sprintf(key, "autocomplete.%s.ignorecase", "*");
	sval = props->GetNewExpand(key, "");
	autoCompleteIgnoreCase = sval == "1";
	sprintf(key, "autocomplete.%s.ignorecase", language.c_str());
	sval = props->GetNewExpand(key, "");
	if (sval != "")
		autoCompleteIgnoreCase = sval == "1";
	SendEditor(SCI_AUTOCSETIGNORECASE, autoCompleteIgnoreCase ? 1 : 0);

	int autoCChooseSingle = props->GetInt("autocomplete.choose.single");
	SendEditor(SCI_AUTOCSETCHOOSESINGLE, autoCChooseSingle),

	SendEditor(SCI_AUTOCSETCANCELATSTART, 0),
	SendEditor(SCI_AUTOCSETDROPRESTOFWORD, 0),

	// Set styles
	// For each window set the global default style,
	// then the language default style,
	// then the other global styles,
	// then the other language styles

	SendEditor(SCI_STYLERESETDEFAULT, 0, 0);

	sprintf(key, "style.%s.%0d", "*", STYLE_DEFAULT);
	sval = props->GetNewExpand(key, "");
	SetOneStyle(wEditor, STYLE_DEFAULT, sval.c_str());

	sprintf(key, "style.%s.%0d", language.c_str(), STYLE_DEFAULT);
	sval = props->GetNewExpand(key, "");
	SetOneStyle(wEditor, STYLE_DEFAULT, sval.c_str());

	SendEditor(SCI_STYLECLEARALL, 0, 0);

	SetStyleFor(wEditor, "*");
	SetStyleFor(wEditor, language.c_str());

	if (firstPropertiesRead) {
		ReadPropertiesInitial();
	}

	/* Gtk handles it correctly */
	SendEditor(SCI_SETUSEPALETTE, 0);
	
	SendEditor(SCI_SETPRINTMAGNIFICATION, props->GetInt("print.magnification"));
	SendEditor(SCI_SETPRINTCOLOURMODE, props->GetInt("print.colour.mode"));

	int blankMarginLeft = props->GetInt("blank.margin.left", 1);
	int blankMarginRight = props->GetInt("blank.margin.right", 1);
	SendEditor(SCI_SETMARGINLEFT, 0, blankMarginLeft);
	SendEditor(SCI_SETMARGINRIGHT, 0, blankMarginRight);

	SendEditor(SCI_SETMARGINWIDTHN, 1, margin ? marginWidth : 0);
	SendEditor(SCI_SETMARGINWIDTHN, 0, lineNumbers ?lineNumbersWidth : 0);

	bufferedDraw = props->GetInt("buffered.draw", 1);
	SendEditor(SCI_SETBUFFEREDDRAW, bufferedDraw);

	SendEditor(SCI_SETLAYOUTCACHE, props->GetInt("cache.layout"));

	bracesCheck = props->GetInt("braces.check");
	bracesSloppy = props->GetInt("braces.sloppy");

	wordCharacters = props->GetNewExpand("word.characters.", fileNameForExtension.c_str());
	if (wordCharacters.length()) {
		SendEditorString(SCI_SETWORDCHARS, 0, wordCharacters.c_str());
	} else {
		SendEditor(SCI_SETWORDCHARS, 0, 0);
		wordCharacters = "_abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
	}
	// Why call this??
	// SendEditor(SCI_MARKERDELETEALL, static_cast<unsigned long>( -1));

	SendEditor(SCI_SETTABINDENTS, props->GetInt("tab.indents", 1));
	SendEditor(SCI_SETBACKSPACEUNINDENTS, props->GetInt("backspace.unindents", 1));

	SendEditor(SCI_SETUSETABS, props->GetInt("use.tabs", 1));
	int tabSize = props->GetInt("tabsize");
	if (tabSize) {
		SendEditor(SCI_SETTABWIDTH, tabSize);
	}
	indentSize = props->GetInt("indent.size");
	SendEditor(SCI_SETINDENT, indentSize);
	/*
	indentOpening = props->GetInt("indent.opening");
	indentClosing = props->GetInt("indent.closing");
	*/
	indentMaintain = props->GetNewExpand("indent.maintain.", fileNameForExtension.c_str()).value();
	/*
	SString lookback = props->GetNewExpand("statement.lookback.", fileNameForExtension.c_str());
	statementLookback = lookback.value();
	statementIndent = GetStyleAndWords("statement.indent.");
	statementEnd =GetStyleAndWords("statement.end.");
	blockStart = GetStyleAndWords("block.start.");
	blockEnd = GetStyleAndWords("block.end.");
	*/
	
	/*
	SString list;
	list = props->GetNewExpand("preprocessor.symbol.", fileNameForExtension.c_str());
	preprocessorSymbol = list[0];
	list = props->GetNewExpand("preprocessor.start.", fileNameForExtension.c_str());
	preprocCondStart.Clear();
	preprocCondStart.Set(list.c_str());
	list = props->GetNewExpand("preprocessor.middle.", fileNameForExtension.c_str());
	preprocCondMiddle.Clear();
	preprocCondMiddle.Set(list.c_str());
	list = props->GetNewExpand("preprocessor.end.", fileNameForExtension.c_str());
	preprocCondEnd.Clear();
	preprocCondEnd.Set(list.c_str());
	*/
	
	if (props->GetInt("vc.home.key", 1)) {
		AssignKey(SCK_HOME, 0, SCI_VCHOME);
		AssignKey(SCK_HOME, SCMOD_SHIFT, SCI_VCHOMEEXTEND);
	} else {
		AssignKey(SCK_HOME, 0, SCI_HOME);
		AssignKey(SCK_HOME, SCMOD_SHIFT, SCI_HOMEEXTEND);
	}
	if (props->GetInt("fold.underline"))
		SendEditor(SCI_SETFOLDFLAGS, props->GetInt("fold.flags"));
	else
		SendEditor(SCI_SETFOLDFLAGS, 0);
	
	// To put the folder markers in the line number region
	//SendEditor(SCI_SETMARGINMASKN, 0, SC_MASK_FOLDERS);

	SendEditor(SCI_SETMODEVENTMASK, SC_MOD_CHANGEFOLD);

	if (0==props->GetInt("undo.redo.lazy")) {
		// Trap for insert/delete notifications (also fired by undo
		// and redo) so that the buttons can be enabled if needed.
		SendEditor(SCI_SETMODEVENTMASK, SC_MOD_INSERTTEXT | SC_MOD_DELETETEXT
			| SC_LASTSTEPINUNDOREDO | SendEditor(SCI_GETMODEVENTMASK, 0));

		//SC_LASTSTEPINUNDOREDO is probably not needed in the mask; it
		//doesn't seem to fire as an event of its own; just modifies the
		//insert and delete events.
	}

	// Create a margin column for the folding symbols
	SendEditor(SCI_SETMARGINTYPEN, 2, SC_MARGIN_SYMBOL);

	SendEditor(SCI_SETMARGINWIDTHN, 2, foldMargin ? foldMarginWidth : 0);

	SendEditor(SCI_SETMARGINMASKN, 2, SC_MASK_FOLDERS);
	SendEditor(SCI_SETMARGINSENSITIVEN, 1, 1); // Breakpoints-Bookmarks
	SendEditor(SCI_SETMARGINSENSITIVEN, 2, 1);
	
	SString fold_symbols = props->Get("fold.symbols");
	SetFoldSymbols (fold_symbols);
	
	// Well, unlike scite, we want it everytime.
	firstPropertiesRead = true;
}

void AnEditor::SetFoldSymbols(SString fold_symbols)
{
	if (fold_symbols.length() <= 0)
		fold_symbols = "plus/minus";
	if (strcasecmp(fold_symbols.c_str(), "arrows") == 0)
	{
		// Arrow pointing right for contracted folders, arrow pointing down for expanded
		DefineMarker(SC_MARKNUM_FOLDEROPEN, SC_MARK_ARROWDOWN, Colour(0, 0, 0), Colour(0, 0, 0));
		DefineMarker(SC_MARKNUM_FOLDER, SC_MARK_ARROW, Colour(0, 0, 0), Colour(0, 0, 0));
		DefineMarker(SC_MARKNUM_FOLDERSUB, SC_MARK_EMPTY, Colour(0, 0, 0), Colour(0, 0, 0));
		DefineMarker(SC_MARKNUM_FOLDERTAIL, SC_MARK_EMPTY, Colour(0, 0, 0), Colour(0, 0, 0));
		DefineMarker(SC_MARKNUM_FOLDEREND, SC_MARK_EMPTY, Colour(0xff, 0xff, 0xff), Colour(0, 0, 0));
		DefineMarker(SC_MARKNUM_FOLDEROPENMID, SC_MARK_EMPTY, Colour(0xff, 0xff, 0xff), Colour(0, 0, 0));
		DefineMarker(SC_MARKNUM_FOLDERMIDTAIL, SC_MARK_EMPTY, Colour(0xff, 0xff, 0xff), Colour(0, 0, 0));
	} else if (strcasecmp(fold_symbols.c_str(), "circular") == 0) {
		// Like a flattened tree control using circular headers and curved joins
		DefineMarker(SC_MARKNUM_FOLDEROPEN, SC_MARK_CIRCLEMINUS, Colour(0xff, 0xff, 0xff), Colour(0x40, 0x40, 0x40));
		DefineMarker(SC_MARKNUM_FOLDER, SC_MARK_CIRCLEPLUS, Colour(0xff, 0xff, 0xff), Colour(0x40, 0x40, 0x40));
		DefineMarker(SC_MARKNUM_FOLDERSUB, SC_MARK_VLINE, Colour(0xff, 0xff, 0xff), Colour(0x40, 0x40, 0x40));
		DefineMarker(SC_MARKNUM_FOLDERTAIL, SC_MARK_LCORNERCURVE, Colour(0xff, 0xff, 0xff), Colour(0x40, 0x40, 0x40));
		DefineMarker(SC_MARKNUM_FOLDEREND, SC_MARK_CIRCLEPLUSCONNECTED, Colour(0xff, 0xff, 0xff), Colour(0x40, 0x40, 0x40));
		DefineMarker(SC_MARKNUM_FOLDEROPENMID, SC_MARK_CIRCLEMINUSCONNECTED, Colour(0xff, 0xff, 0xff), Colour(0x40, 0x40, 0x40));
		DefineMarker(SC_MARKNUM_FOLDERMIDTAIL, SC_MARK_TCORNERCURVE, Colour(0xff, 0xff, 0xff), Colour(0x40, 0x40, 0x40));
	} else if (strcasecmp(fold_symbols.c_str(), "squares") == 0) {
		// Like a flattened tree control using square headers
		DefineMarker(SC_MARKNUM_FOLDEROPEN, SC_MARK_BOXMINUS, Colour(0xff, 0xff, 0xff), Colour(0x80, 0x80, 0x80));
		DefineMarker(SC_MARKNUM_FOLDER, SC_MARK_BOXPLUS, Colour(0xff, 0xff, 0xff), Colour(0x80, 0x80, 0x80));
		DefineMarker(SC_MARKNUM_FOLDERSUB, SC_MARK_VLINE, Colour(0xff, 0xff, 0xff), Colour(0x80, 0x80, 0x80));
		DefineMarker(SC_MARKNUM_FOLDERTAIL, SC_MARK_LCORNER, Colour(0xff, 0xff, 0xff), Colour(0x80, 0x80, 0x80));
		DefineMarker(SC_MARKNUM_FOLDEREND, SC_MARK_BOXPLUSCONNECTED, Colour(0xff, 0xff, 0xff), Colour(0x80, 0x80, 0x80));
		DefineMarker(SC_MARKNUM_FOLDEROPENMID, SC_MARK_BOXMINUSCONNECTED, Colour(0xff, 0xff, 0xff), Colour(0x80, 0x80, 0x80));
		DefineMarker(SC_MARKNUM_FOLDERMIDTAIL, SC_MARK_TCORNER, Colour(0xff, 0xff, 0xff), Colour(0x80, 0x80, 0x80));
	} else { // Default
		// Plus for contracted folders, minus for expanded
		DefineMarker(SC_MARKNUM_FOLDEROPEN, SC_MARK_MINUS, Colour(0xff, 0xff, 0xff), Colour(0, 0, 0));
		DefineMarker(SC_MARKNUM_FOLDER, SC_MARK_PLUS, Colour(0xff, 0xff, 0xff), Colour(0, 0, 0));
		DefineMarker(SC_MARKNUM_FOLDERSUB, SC_MARK_EMPTY, Colour(0xff, 0xff, 0xff), Colour(0, 0, 0));
		DefineMarker(SC_MARKNUM_FOLDERTAIL, SC_MARK_EMPTY, Colour(0xff, 0xff, 0xff), Colour(0, 0, 0));
		DefineMarker(SC_MARKNUM_FOLDEREND, SC_MARK_EMPTY, Colour(0xff, 0xff, 0xff), Colour(0, 0, 0));
		DefineMarker(SC_MARKNUM_FOLDEROPENMID, SC_MARK_EMPTY, Colour(0xff, 0xff, 0xff), Colour(0, 0, 0));
		DefineMarker(SC_MARKNUM_FOLDERMIDTAIL, SC_MARK_EMPTY, Colour(0xff, 0xff, 0xff), Colour(0, 0, 0));
	}
}

// Anjuta: In our case, we read it everytime
void AnEditor::ReadPropertiesInitial() {
	indentationWSVisible = props->GetInt("view.indentation.whitespace", 1);
	ViewWhitespace(props->GetInt("view.whitespace"));
	SendEditor(SCI_SETINDENTATIONGUIDES, props->GetInt("view.indentation.guides"));
	SendEditor(SCI_SETVIEWEOL, props->GetInt("view.eol"));

	SetReadOnly(props->GetInt("file.readonly", 0));
	SetLineWrap(props->GetInt("view.line.wrap", 1));

	//lineNumbersWidth = 0;
	/* FIXME: This is nowhere configureable
	SString linenums = props->Get("margin.linenumber.width");
	if (linenums.length())
		lineNumbersWidth = linenums.value(); */
	//lineNumbers = lineNumbersWidth;
	/* We do this dynamicly in text_editor_load_file now */
	/* if (lineNumbersWidth == 0)
		lineNumbersWidth = lineNumbersWidthDefault;*/
			
	marginWidth = 0;
	SString margwidth = props->Get("margin.marker.width");
	if (margwidth.length())
		marginWidth = margwidth.value();
	margin = marginWidth;
	if (marginWidth == 0)
		marginWidth = marginWidthDefault;

	foldMarginWidth = props->GetInt("margin.fold.width", foldMarginWidthDefault);
	foldMargin = foldMarginWidth;
	if (foldMarginWidth == 0)
		foldMarginWidth = foldMarginWidthDefault;

	lineNumbers = props->GetInt("margin.linenumber.visible", 0);
	SendEditor(SCI_SETMARGINWIDTHN, 0, lineNumbers ? lineNumbersWidth : 0);
	margin = props->GetInt("margin.marker.visible", 0);
	SendEditor(SCI_SETMARGINWIDTHN, 1, margin ? marginWidth : 0);

	foldMargin = props->GetInt("margin.fold.visible", 1);
	SendEditor(SCI_SETMARGINWIDTHN, 2, foldMargin ? foldMarginWidth : 0);
}

void AnEditor::DefineMarker(int marker, int markerType, Colour fore, Colour back)
{
	SendEditor(SCI_MARKERDEFINE, marker, markerType);
	SendEditor(SCI_MARKERSETFORE, marker, fore.AsLong());
	SendEditor(SCI_MARKERSETBACK, marker, back.AsLong());
}

int AnEditor::GetBookmarkLine( const int nLineStart )
{
	int nNextLine = SendEditor(SCI_MARKERNEXT, nLineStart+1, 1 << ANE_MARKER_BOOKMARK);
	//printf( "...look %d --> %d\n", nLineStart, nNextLine );
	if ( (nNextLine < 0) || (nNextLine == nLineStart) )
		return -1 ;
	else
		return nNextLine;
}

void AnEditor::FocusInEvent(GtkWidget* widget)
{
	if (calltipShown)
	{
		StartCallTip_new();
	}
}

void AnEditor::FocusOutEvent(GtkWidget* widget)
{
	if (SendEditor(SCI_CALLTIPACTIVE))
	{
		SendEditor(SCI_CALLTIPCANCEL);
		calltipShown = true;
	}
	else
	{
		calltipShown = false;
	}	
}

static GList* editors;

static AnEditor*
aneditor_get(AnEditorID id)
{
	AnEditor* ed;
	if(id >= g_list_length(editors))
	{
		DEBUG_PRINT("%s", "Invalid AnEditorID supplied");
		return NULL;
	}
	ed = (AnEditor*)g_list_nth_data(editors, (guint)id);
	if(!ed)
	{
		DEBUG_PRINT("%s", "Trying to use already destroyed AnEditor Object");
		return NULL;
	}
	return ed;
}

AnEditorID
aneditor_new(gpointer propset)
{
  AnEditor* ed = new AnEditor((PropSetFile*)propset);
  if (!ed)
  {
     DEBUG_PRINT("%s", "Memory allocation error.");
     return ANE_ID_INVALID;
  }
  g_signal_connect(ed->GetID(), "focus_in_event", 
				   G_CALLBACK(on_aneditor_focus_in), ed);
  g_signal_connect(ed->GetID(), "focus_out_event", 
				   G_CALLBACK(on_aneditor_focus_out), ed);
  editors = g_list_append(editors, ed);
  return (AnEditorID)(g_list_length(editors) - 1);
}

void
aneditor_destroy(AnEditorID id)
{
  AnEditor* ed;

  ed = aneditor_get(id);
  if(!ed) return;
  
  /* We will not remove the editor from the list */
  /* so that already assigned handles work properly */
  /* We'll simply make it NULL to indicate that the */
  /* editor is destroyed */
  g_list_nth(editors, id)->data = NULL;
	
  /* Disconnect the focus in/out signals */
  g_signal_handlers_disconnect_by_func (ed->GetID(),
				   (void*)G_CALLBACK(on_aneditor_focus_in), ed);
  g_signal_handlers_disconnect_by_func (ed->GetID(),
				   (void*)G_CALLBACK(on_aneditor_focus_out), ed);

  delete ed;
}

GtkWidget*
aneditor_get_widget(AnEditorID handle)
{
   AnEditor *ed;
   ed = aneditor_get(handle);
   if(!ed) return NULL;

   // Forced conversion is safe here, so do it.
   return (GtkWidget*)ed->GetID();
}

glong
aneditor_command(AnEditorID handle, gint command, glong wparam, glong lparam)
{
   AnEditor *ed;
   ed = aneditor_get(handle);
   if(!ed) return 0;
   return ed->Command(command, wparam, lparam);
}

void
aneditor_set_focused_ed_ID(AnEditorID id)
{
	AnEditor::focusedID = id;
}

void
aneditor_set_parent(AnEditorID id, AnEditorID parent_id)
{
	AnEditor *editor;
	AnEditor *parent;
	
	editor = aneditor_get (id);
	parent = aneditor_get (parent_id);
	editor->SetParent(parent);
}

gint
on_aneditor_focus_in (GtkWidget* widget, gpointer* unused, AnEditor* ed)
{
	ed->FocusInEvent(widget);
	return FALSE;
}

gint
on_aneditor_focus_out (GtkWidget* widget, gpointer * unused, AnEditor* ed)
{
	/* ed->EndDebugEval(); */
	ed->FocusOutEvent(widget);
	return FALSE;
}
