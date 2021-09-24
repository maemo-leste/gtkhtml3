
#ifndef __gtkhtml_editor_marshal_MARSHAL_H__
#define __gtkhtml_editor_marshal_MARSHAL_H__

#include	<glib-object.h>

G_BEGIN_DECLS

/* VOID:STRING,POINTER (gtkhtml-editor-marshal.list:1) */
extern void gtkhtml_editor_marshal_VOID__STRING_POINTER (GClosure     *closure,
                                                         GValue       *return_value,
                                                         guint         n_param_values,
                                                         const GValue *param_values,
                                                         gpointer      invocation_hint,
                                                         gpointer      marshal_data);

/* STRING:STRING (gtkhtml-editor-marshal.list:2) */
extern void gtkhtml_editor_marshal_STRING__STRING (GClosure     *closure,
                                                   GValue       *return_value,
                                                   guint         n_param_values,
                                                   const GValue *param_values,
                                                   gpointer      invocation_hint,
                                                   gpointer      marshal_data);

G_END_DECLS

#endif /* __gtkhtml_editor_marshal_MARSHAL_H__ */

