#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "../../INCLUDE/const.h"
#include "../../INCLUDE/struct.h"
#include "../../INCLUDE/var_ext.h"
#include "../../INCLUDE/fun_ext.h"
#include "../../INCLUDE/macros.h"
#include "../../INCLUDE/service.h"


#define TGSPN 1

#ifdef REACHABILITY


extern Throughput_p fill_throughput_node();
extern MRate_p fill_mark_node();
extern PComp_p fill_factor();
extern  void store_compact();
extern  void load_compact();
extern  void store_double();
extern  void load_double();
extern void reset_to_M0();

extern void initialize_en_list();
extern void update_en_list();
#ifdef LIBSPOT
extern int is_enabled();
#endif
extern void fire_trans();
extern void push_result();
//MDWN
#ifdef MDWN
extern void write_cvrs(FILE *, FILE *, int);
extern int cvrs_flag;
extern int CVRS_flag;
extern FILE *f_cvrs, *f_cvrsoff;
extern FILE *f_CVRS;
#endif
//MDWN
extern void write_ctrs();
extern void write_grg();
extern void write_on_srg();
extern void write_final_results();
extern void update_rg_files(int mark_type, Tree_p fire, int tr, int pri);
extern void update_rg_rate(double rate);
extern void out_cur_marking();
extern void out_single_instance();
extern void string_to_marking();
extern double get_instance_rate();
extern unsigned long marking_to_string();

extern MRate_p rate_pop();
extern PComp_p path_pop();
extern void push_path_element();
extern void push_path_list();

extern Event_p get_new_event();
extern int invoked_from_gui();

static Tree_p fire_ptr = NULL;	 /* Puntatore alla marcatura di sparo	 */
static Tree_p top = NULL;	 /* Puntatori al top e al bottom dello	 */
static Tree_p bottom = NULL;	 /* Stack di marcature da espandere	 */
static Tree_p root = NULL;	 /* Albero delle marcature raggiunte	 */

static int err_fseek;

Tree_p reached_marking = NULL;
Tree_p initial_marking = NULL;
Tree_p current_marking = NULL;

Result_p enabled_head = NULL;	/* Puntatori alla lista delle tr. abil. */

int tro;			/* 1 marc. gia' raggiunta, 0 altrimenti */
int marcatura = 0;	/* contatore per le marcature		*/
int h = 0;			/* per bilanciamento nella insert_tree	*/
unsigned long tang = 0;		/* contatori tipi di marc. raggiunte	*/
unsigned long evan = 0;
unsigned long dead = 0;
int home = 0;
unsigned long cont_tang;
extern long g_max_markings_built;
int count_arc = 0;
#ifdef SYMBOLIC
extern void create_canonical_data_structure();
extern void get_canonical_marking();

double mark_ordinarie;	    /* num. marc. ord. per marc. simbolica  */
double ord_tang = 0.0;
double ord_evan = 0.0;
double ord_dead = 0.0;

extern Canonic_p sfl_h;
#endif

int cur_priority;

int *code_place = NULL;
int *min_place = NULL;
int *max_place = NULL;
int *init_place = NULL;
char IstanceName[MAX_TAG_SIZE];
char tmpIstanceName[MAX_TAG_SIZE];
int max_priority = 0;
time_t old_time, new_time;
int continue_build = TRUE;

static MRate_p path_head_ptr = NULL; static unsigned long c_ph = 0;
static MRate_p path_tail_ptr = NULL;

static MRate_p tangible_path_head_ptr = NULL; static unsigned long c_tph = 0;
static MRate_p tangible_path_tail_ptr = NULL;

static Throughput_p throu_head_ptr = NULL; static unsigned long c_th = 0;
static Throughput_p throu_tail_ptr = NULL;

static Throughput_p total_throu_head_ptr = NULL; static unsigned long c_tth = 0;
static Throughput_p total_throu_tail_ptr = NULL;

static unsigned long in_tr;
static unsigned long in_enabling_degree;
static unsigned long in_ordinary;
static unsigned long in_denom_p;


static unsigned long tot_path = 0;

unsigned long d_ptr;
unsigned long length;
unsigned long f_mark;
int f_bot;
int f_throu;
int f_tang;

extern int out_mc;
extern int exp_set;
extern int fast_solve;
extern int dot_flag;

//MDWN
#ifdef MDWN
extern int MDWN_flag;
extern FILE *f_MDWN;
int non_det = 0; //use to identify when the system is in the non-deterministic part
//int firstmarking=0;
int pp_cc = 0;
extern  double Threshold;
#endif
//MDWN

//TGSPN
#ifdef TGSPN
extern int tgspn;
extern char TaggedTok[MAX_BUFFER]; //tagged Token id
extern int num_Tplace;//|places|
extern int *Tplace;//places' list
extern int num_TtranS;//|start transition|
extern int *TtranS;//start transitions' list
extern int num_TtranE;//|end transition|
extern int *TtranE;//end transitions' list
extern int num_TtranF;//|forbidden transition|
extern int *TtranF;//forbidden transitions' list
extern FILE *f_outtgspn; //TGSPN output file
int tranS = FALSE;
int tranE = FALSE;
int tranF = FALSE;
int instSEF = FALSE; //transition instance
int PreCond = FALSE;
#endif
//TGSPN
#ifdef SWN
#ifdef SYMBOLIC
/**************************************************************/
/* NAME : get_static_subclass_name*/
/* DESCRIPTION : this fuction returns the static subclass name */
/* PARAMETERS :int  cl,  int  dyn_num,  char  name[MAX_TAG_SIZE]*/
/* RETURN VALUE : */
/**************************************************************/
void get_static_subclass_name(int  cl,  int  dyn_num,  char  name[MAX_TAG_SIZE]) {
    int sb;
    sb = get_static_subclass(cl, dyn_num);
    sprintf(name, "%s", GET_STATIC_NAME(cl, sb));
}
#endif
#endif


/**************************************************************/
static void check_continue_build() {
    if (tang + evan >= g_max_markings_built) {
        printf("Stop RG construction at %d markings. Reached max marking bound.\n", tang + evan);
        continue_build = FALSE; // We have built enough markings. Stop RG construction.
    }
}

int exceeded_markings_bound() {
    if (g_max_markings_built <= 0)
        return FALSE; // No bound.
    return (tang+evan >= g_max_markings_built);
}

#ifdef TGSPN
/**************************************************************/
/* NAME : checkTtrans*/
/* DESCRIPTION : this fuction check if tr is in TtranS/TtranE/TtranF*/
/* PARAMETERS : int& tranS,int& tranE,int& tranF,int tr*/
/* RETURN VALUE : */
/**************************************************************/
void checkTtrans(int *tranS, int *tranE, int *tranF, int tr) {
    int i = 0;
    while ((i < num_TtranS) && (!(*tranS))) {
        if (TtranS[i] == tr) {
            (*tranS) = TRUE;
        }
        i++;
    }
    i = 0;
    while ((i < num_TtranE) && (!(*tranE))) {
        if (TtranE[i] == tr) {
            (*tranE) = TRUE;
        }
        i++;
    }
    i = 0;
    while ((i < num_TtranF) && (!(*tranF))) {
        if (TtranF[i] == tr) {
            (*tranF) = TRUE;
        }
        i++;
    }

}
/**************************************************************/
/* NAME : checkTplaces*/
/* DESCRIPTION : this fuction check if the tagged token is in Tplaces*/
/* PARAMETERS : */
/* RETURN VALUE : */
/**************************************************************/
int checkTplaces(void) {
    int pl, k, cl, i = 0;
#ifdef SWN
#ifdef SYMBOLIC
    int offset, sb, base;
#endif
#endif
    int found = FALSE;
    char obj_name[MAX_TAG_SIZE];
#ifdef SWN
    while ((i < num_Tplace) && (!found)) {
        /* Per ogni posto in Tplace*/
        pl = Tplace[i];
        if (IS_FULL(pl)) {
            /* Posto pieno */
            if (!IS_NEUTRAL(pl)) {
                /* Posto colorato */
                Token_p tk_p;
                for (tk_p = net_mark[pl].marking; tk_p != NULL; tk_p = NEXT_TOKEN(tk_p)) {
                    /* Per ogni token della marcatura */
                    for (k = 0 ; k < tabp[pl].comp_num - 1; k++) {
                        cl = tabp[pl].dominio[k];
#ifdef COLOURED
                        get_object_name(cl, tk_p->id[k], obj_name);
#endif
#ifdef SYMBOLIC

                        /*if(IS_UNORDERED(cl) || (IS_ORDERED(cl) && GET_STATIC_SUBCLASS(cl) == 1))
                        get_dynamic_subclass_name(cl,tk_p->id[k],obj_name);
                        else
                        	get_object_name(cl,tk_p->id[k],obj_name);*/
                        get_static_subclass_name(cl, tk_p->id[k], obj_name);
#endif
                        if ((!found) && (strcmp(TaggedTok, obj_name) == 0)) {
                            found = TRUE;
                        }
                        obj_name[0] = '\0';
                    }
                    cl = tabp[pl].dominio[k];
#ifdef COLOURED
                    get_object_name(cl, tk_p->id[k], obj_name);
#endif
#ifdef SYMBOLIC
                    /*if(IS_UNORDERED(cl) || (IS_ORDERED(cl) && GET_STATIC_SUBCLASS(cl) == 1))
                    get_dynamic_subclass_name(cl,tk_p->id[k],obj_name);
                    else
                    get_object_name(cl,tk_p->id[k],obj_name);*/
                    get_static_subclass_name(cl, tk_p->id[k], obj_name);
#endif

                    if ((!found) && (strcmp(TaggedTok, obj_name) == 0)) {
                        found = TRUE;
                    }
                }
            }
            else {
                //NEUTRAL
                found = TRUE;
            }//NEUTRAL
        }/* Posto pieno */
        i++;
    }/* Per ogni posto in Tplace*/
#endif
    return found;
}
//TGSPN
#endif


//MDWN&TGSPN
/**************************************************************/
/* NAME : get_instance*/
/* DESCRIPTION : this fuction gets the transition instance*/
/* PARAMETERS : Event_p  ev_p,  FILE  *fp, FILE *out*/
/* RETURN VALUE : */
/**************************************************************/
void get_instance(Event_p  ev_p) {
    /* Init out_single_instance */
    int tr = GET_TRANSITION_INDEX(ev_p);
    int ii;
    int cl;
#ifdef TGSPN
//TGSPN
    instSEF = FALSE;
//TGSPN
#endif

#ifdef SWN
    if (GET_TRANSITION_COMPONENTS(tr) > 0) {
        /* Transizione colorata */
        cl = tabt[tr].dominio[0];
        if (ev_p->npla[0] != UNKNOWN) {
#ifdef COLOURED
            get_object_name(cl, ev_p->npla[0], IstanceName);
#endif
#ifdef SYMBOLIC
            if (tgspn) {
                get_static_subclass_name(cl, ev_p->npla[0], IstanceName);
            }
            else {
                if (IS_UNORDERED(cl) || (IS_ORDERED(cl) && GET_STATIC_SUBCLASS(cl) == 1))
                    get_dynamic_subclass_name(cl, ev_p->npla[0], IstanceName);
                else
                    get_object_name(cl, ev_p->npla[0], IstanceName);
            }
#ifdef SIMULATION
            if (IS_UNORDERED(cl))
                get_dynamic_subclass_name(cl, ev_p->npla[0], IstanceName);
            else
                get_object_name(cl, ev_p->npla[0], IstanceName);
#endif
#endif
//TGSPN
#ifdef TGSPN
            if ((tgspn)) {
                if (((tranS) || (tranE) || (tranF)) && (!instSEF) && (strcmp(TaggedTok, IstanceName) == 0)) {
                    instSEF = TRUE;
                }
                IstanceName[0] = '\0';
            }
//TGSPN
#endif
        }
        else
            strcpy(IstanceName, "Unk");
        for (ii = 1; ii < tabt[tr].comp_num; ii++) {
            cl = tabt[tr].dominio[ii];
            if (ev_p->npla[ii] != UNKNOWN) {
#ifdef COLOURED
                get_object_name(cl, ev_p->npla[ii], tmpIstanceName);
                strcat(IstanceName, tmpIstanceName);
#endif
#ifdef SYMBOLIC

#ifdef TGSPN
//TGSPN
                if (tgspn) {
                    get_static_subclass_name(cl, ev_p->npla[0], IstanceName);
                }
                else {
                    if (IS_UNORDERED(cl) || (IS_ORDERED(cl) && GET_STATIC_SUBCLASS(cl) == 1))
                        get_dynamic_subclass_name(cl, ev_p->npla[ii], tmpIstanceName);
                    else
                        get_object_name(cl, ev_p->npla[ii], tmpIstanceName);
                    strcat(IstanceName, tmpIstanceName);
                }
#else
                if (IS_UNORDERED(cl) || (IS_ORDERED(cl) && GET_STATIC_SUBCLASS(cl) == 1))
                    get_dynamic_subclass_name(cl, ev_p->npla[ii], IstanceName);
                else
                    get_object_name(cl, ev_p->npla[ii], IstanceName);
                strcat(IstanceName, tmpIstanceName);
#endif

#ifdef SIMULATION
                if (IS_UNORDERED(cl))
                    get_dynamic_subclass_name(cl, ev_p->npla[ii], IstanceName);
                else
                    get_object_name(cl, ev_p->npla[ii], IstanceName);
#endif
#endif
            }
            else
                strcpy(IstanceName, "Unk");

#ifdef TGSPN
//TGSPN
            if ((tgspn)) {
                if (((tranS) || (tranE) || (tranF)) && (!instSEF) && (strcmp(TaggedTok, IstanceName) == 0)) {
                    instSEF = TRUE;
                }
                IstanceName[0] = '\0';
            }
//TGSPN
#endif
        }
    }/* Transizione colorata */
#endif
#ifdef TGSPN
//TGSPN
    if ((tgspn)) {
        IstanceName[0] = '\0';
    }
//TGSPN
#endif
}/* End out_single_instance */
//MDWN




/**************************************************************/
/* NAME : */
/* DESCRIPTION : */
/* PARAMETERS : */
/* RETURN VALUE : */
/**************************************************************/
static void arrcpy(int *s ,  int *t,  int lim) {
    /* Init arrcpy */
    int i;

    for (i = lim ; i ; i--)
        s[i - 1] = t[i - 1];
}/* End arrcpy */
/**************************************************************/
/* NAME : */
/* DESCRIPTION : */
/* PARAMETERS : */
/* RETURN VALUE : */
/**************************************************************/
static unsigned long append_to_list_of_throu(Throughput_p *head, Throughput_p *tail, Throughput_p add_head, Throughput_p add_tail) {
    /* Init append_to_list_of_throu */
    Throughput_p list = NULL;
    Throughput_p cur = NULL;
    Throughput_p nxt = NULL;
    int found = FALSE;
    unsigned long appended = 0;

    if (*head == NULL && *tail == NULL) {
        *head = add_head;
        *tail = add_tail;
        for (cur = add_head; cur != NULL; cur = cur->next, appended++);
    }
    else {
        if (add_head != NULL && add_tail != NULL) {
            if (exp_set) {
                /* Opzione per set di esperimenti */
                (*tail)->next = add_head;
                *tail = add_tail;
                for (cur = add_head; cur != NULL; cur = cur->next, appended++);
            }/* Opzione per set di esperimenti */
            if (!exp_set) {
                /* Opzione per unico run */
                for (cur = add_head; cur != NULL; cur = nxt) {
                    /* Per ogni elemento da aggiungere */
                    nxt = cur->next;
                    found = FALSE;
                    for (list = *head; list != NULL; list = list->next)
                        if (cur->tr == list->tr) {
                            found = TRUE;
                            list->weight += cur->weight;
                            push_throu_element(cur);
                            goto finish;
                        }
finish:   if (found == FALSE) {
                        (*tail)->next = cur;
                        *tail = cur;
                        (*tail)->next = NULL;
                        appended++;
                    }
                }/* Per ogni elemento da aggiungere */
            }/* Opzione per unico run */
        }
    }
    if (*tail != NULL)
        (*tail)->next = NULL;
    return (appended);
}/* End append_to_list_of_throu */
/**************************************************************/
/* NAME : */
/* DESCRIPTION : */
/* PARAMETERS : */
/* RETURN VALUE : */
/**************************************************************/
static unsigned long append_to_list_of_tangible_reachable(MRate_p *head, MRate_p *tail, MRate_p add_head, MRate_p add_tail) {
    /* Init append_to_list_of_tangible_reachable */
    MRate_p list = NULL;
    MRate_p cur = NULL;
    MRate_p nxt = NULL;
    int found = FALSE;
    unsigned long appended = 0;

    if (*head == NULL && *tail == NULL) {
        *head = add_head;
        *tail = add_tail;
        for (cur = add_head; cur != NULL; cur = cur->next, appended++);
    }
    else {
        if (add_head != NULL && add_tail != NULL) {
            if (exp_set) {
                /* Opzione per set di esperimenti */
                (*tail)->next = add_head;
                *tail = add_tail;
                for (cur = add_head; cur != NULL; cur = cur->next, appended++);
            }/* Opzione per set di esperimenti */
            if (!exp_set) {
                /* Opzione per unico run */
                for (cur = add_head; cur != NULL; cur = nxt) {
                    /* Per ogni elemento da aggiungere */
                    nxt = cur->next;
                    found = FALSE;
                    for (list = *head; list != NULL; list = list->next)
                        if (cur->cont_tang == list->cont_tang && cur->flag == list->flag) {
                            found = TRUE;
                            list->mean_t += cur->mean_t;
                            push_rate_element(cur);
                            goto finish;
                        }
finish:   if (found == FALSE) {
                        (*tail)->next = cur;
                        *tail = cur;
                        (*tail)->next = NULL;
                        appended++;
                    }
                }/* Per ogni elemento da aggiungere */
            }/* Opzione per unico run */
        }
    }
    if (*tail != NULL)
        (*tail)->next = NULL;
    return (appended);
}/* End append_to_list_of_tangible_reachable */

/**************************************************************/
/* NAME : */
/* DESCRIPTION : */
/* PARAMETERS : */
/* RETURN VALUE : */
/**************************************************************/
static void dispose_all_instances(Result_p res_ptr) {
    /* Init dispose_all_instances */
    Event_p ev_p, nev_p;

    if (res_ptr != NULL) {
        /* Puntatore consistente */
        ev_p = res_ptr->list;
        while (ev_p != NULL) {
            nev_p = ev_p->next;
            dispose_old_event(ev_p);
            ev_p = nev_p;
        }
    }/* Puntatore consistente */
}/* End dispose_all_instances */
/* ********************************************************************* */
/*                                                                       */
/*    Funzioni che permettono il calcolo dei tassi associati alle        */
/*    transizioni abilitate                                              */
/*                                                                       */
/* ********************************************************************* */
static double normalizzazione(Result_p pun, int pri) {
    Result_p tmp = pun;
    double prod;
    double den = 0;
    Event_p ev_p = NULL;
    int tr;
    int tot = 0;

    while (tmp != NULL) {
        /* Per ogni transizione */
        ev_p = tmp->list;
        prod = 0;
        tr = GET_TRANSITION_INDEX(ev_p);
        if (tabt[tr].pri == pri) {
            /* Se sono allo stesso livello di priorita' */
            while (ev_p != NULL) {
                /* Per ogni istanza */
                tot++;
                tr = GET_TRANSITION_INDEX(ev_p);
                prod += get_instance_rate(ev_p) * GET_ENABLING_DEGREE(ev_p);
                ev_p = NEXT_EVENT(ev_p);
            }/* Per ogni istanza */
            den += prod;
        }/* Se sono allo stesso livello di priorita' */
        tmp = tmp->next;
    }/* Per ogni transizione */
    if (exp_set) {
        /* Opzione per set di esperimenti */
        store_compact(tot, denom);
        while (pun != NULL) {
            /* Per ogni transizione */
            ev_p = pun->list;
            tr = GET_TRANSITION_INDEX(ev_p);
            if (tabt[tr].pri == pri) {
                /* Se sono allo stesso livello di priorita' */
                while (ev_p != NULL) {
                    /* Per ogni istanza */
                    store_compact(ev_p->trans, denom);
#ifdef SWN
#ifdef SYMBOLIC
                    store_compact(ev_p->ordinary_instances, denom);
#endif
#endif
                    ev_p = NEXT_EVENT(ev_p);
                }/* Per ogni istanza */
            }/* Se sono allo stesso livello di priorita' */
            pun = pun->next;
        }/* Per ogni transizione */
    }/* Opzione per set di esperimenti */
    return (den);
}
/**************************************************************/
/* NAME : */
/* DESCRIPTION : */
/* PARAMETERS : */
/* RETURN VALUE : */
/**************************************************************/
static void garbage_collect(Tree_p fire_ptr) {
    /* Init garbage_collect */
    Result_p current_transition, next_transition;

    current_transition = fire_ptr->enabled_head;
    while (current_transition != NULL) {
        next_transition = current_transition->next;
        dispose_all_instances(current_transition);
        current_transition->list = NULL;
        push_result(current_transition);
        current_transition = next_transition;
    }
    fire_ptr->enabled_head = NULL;
}/* End garbage_collect */
/**************************************************************/
/* NAME : */
/* DESCRIPTION : */
/* PARAMETERS : */
/* RETURN VALUE : */
/**************************************************************/
void DFS_evanescenti(int  f_mark, double mean) {
    /* Init DFS_evanescenti */
    char cc;
    int tr;
    int marking_pri;

    Result_p current_transition;

    Event_p ev_p, nev_p, copy_of_ev_p;

    Tree_p new_reached_marking;
    Tree_p dfs_fire_ptr;
    PComp_p comp_p = NULL;
    MRate_p pun = NULL;
    MRate_p cur = NULL;
    MRate_p marc_ragg = NULL;
    MRate_p head_marc = NULL; unsigned long  c_hm = 0;
    MRate_p tail_marc = NULL;
    Throughput_p pun_throu = NULL;
    Throughput_p cur_throu = NULL;
    Throughput_p throu_ragg = NULL;
    Throughput_p head_throu = NULL; unsigned long c_ht = 0;
    Throughput_p tail_throu = NULL;
    int tot_p = 0;
    int temp_p ;

    long nmark;
    int nk;
    long st;
    double ra;
    long tnmark;
    long tnk, nfatt;
    long tst;
    int en_degree = 1;
    int ordinary_m = 1;
    unsigned long denom_p = 0;
    double tra;
    double dval;
#ifdef TGSPN
//TGSPN
    int DPreCond = FALSE, PostCond = FALSE;
    int DinstSEF = instSEF;
    int DtranS = 0, DtranE = 0, DtranF = 0;
    int cntTemp = 0;
//TGSPN
#endif

    double denomin = 0, numer, mean_t = 0;

    dfs_fire_ptr = new_reached_marking = reached_marking;
    marking_pri = dfs_fire_ptr->marking->pri;

    if (IS_VANISHING(marking_pri) && out_mc) {
        denom_p = ftell(denom);
        denomin = normalizzazione(dfs_fire_ptr->enabled_head, marking_pri);
    }
    current_transition = dfs_fire_ptr->enabled_head;
    while (current_transition != NULL && continue_build) {
        /* per tutte le transizioni abilitate */
        tr = GET_TRANSITION_INDEX(current_transition->list);

        if (tabt[tr].pri == marking_pri) {
            /* Solo per la stessa priorita' */
            copy_of_ev_p = get_new_event(tr);
            ev_p = current_transition->list;
#ifdef TGSPN
//TGSPN
            if (tgspn) {
                //tranS=tranE=tranF=FALSE; manage transition sequences
                checkTtrans(&DtranS, &DtranE, &DtranF, tr);
            }
//TGSPN
#endif
            while (ev_p != NULL && continue_build) {
                /* per ogni istanza */
                nev_p = ev_p->next;
#ifdef TGSPN
//TGSPN
                if (tgspn) {
                    if ((DtranS) || (DtranE) || (DtranF)) {
                        //check if the transition instance contains the tagged token
                        if (strcmp(TaggedTok, "-") != 0)
                            get_instance(ev_p);
                        else
                            instSEF = TRUE; //NEUTRAL
                        //check if the transition instance contains the tagged token
                        //check the precondition (Obs.  it returns true if the tagged token is in Tplace)
                        if (instSEF == TRUE) {
                            DinstSEF = TRUE;
                            if (DtranS)
                                tranS = DtranS;
                            if (DtranE)
                                tranE = DtranE;
                            if (DtranF)
                                tranF = DtranF;
                            DPreCond = checkTplaces();
                            if (DPreCond)
                                PreCond = DPreCond;
                        }
                    }
                    //PreCond=checkTplaces();
                    //check the precondition
                }
//TGSPN
#endif
                if (out_mc) {
                    /* Construction of Markov Chain */
                    if (ev_p != NULL) {
                        /*en_degree = ev_p->enabling_degree ;*/
#ifdef SWN
#ifdef SYMBOLIC
                        ordinary_m = ev_p->ordinary_instances;
#endif
#endif
                    }
                    double trn_weight = get_instance_rate(ev_p);
#ifdef MDWN
                    // Change the trn_weight to account for uncertainty
                    trn_weight += tabt[ev_p->trans].uncertainty;
#endif
                    numer = en_degree *  ordinary_m * trn_weight;
                    if (IS_VANISHING(marking_pri))
                        mean_t = numer / denomin;
                    else
                        mean_t = numer;

                    update_rg_rate(mean_t * mean);
                }/* Construction of Markov Chain */
                if (output_flag) {
                    /* Scrittura abilitata */
                    update_rg_files(TANGIBLE_OR_VANISHING, dfs_fire_ptr, tr, marking_pri);
                }/* Scrittura abilitata */

                copy_event(copy_of_ev_p, ev_p);
                if (IS_RESET_TRANSITION(tr))
#ifdef SWN
#ifdef SYMBOLIC
                    reset_to_M0(initial_marking->marking->marking_as_string, initial_marking->marking->d_ptr, initial_marking->marking->length, tr);
#endif
#ifdef COLOURED
                reset_to_M0(initial_marking->marking->marking_as_string, initial_marking->marking->length, tr);
#endif
#endif
#ifdef GSPN
                reset_to_M0(initial_marking->marking->marking_as_string, initial_marking->marking->length, tr);
#endif
                else
                    fire_trans(ev_p);
                if (output_flag) {
                    /* Scrittura abilitata */
                    out_single_instance(ev_p, srg);
                    if (out_mc) {
                        /* Construction of Markov Chain */
                        fprintf(srg, "--->   (rate %lg)\n", mean_t);
                    }/* Construction of Markov Chain */
                }/* Scrittura abilitata */
#ifdef SWN
#ifdef SYMBOLIC
                get_canonical_marking();
#endif
#endif
                f_mark = marking_to_string();
                tro = 0;
#ifdef MDWN
                if ((cur_priority) || (!cur_priority) && ((mean_t *mean >= Threshold)) || (non_det)) {
#endif
                    insert_tree(&root, &h, f_mark, length, d_ptr); /* Piu' info nel symbolic */
                    if (current_marking == initial_marking && !home)
                        home = 1;
                    switch (tro) {
                    /* Vari tipi di marcatura raggiunta */
                    case DEAD_OLD:
#ifdef TGSPN
//TGSPN
                        if (tgspn)
                            PostCond = checkTplaces();
//TGSPN
#endif
                        if (output_flag) {
                            /* Scrittura abilitata */
                            update_rg_files(NORMAL, NULL, UNKNOWN, -1);
                        }/* Scrittura abilitata */
                        if (out_mc) {
                            /* Construction of Markov Chain */
                            marc_ragg = fill_mark_node(FALSE, cont_tang, mean_t);
                            if (exp_set) {
                                /* Opzione per set di esperimenti */
                                comp_p = fill_factor(tr, ordinary_m, 1, denom_p);
                                marc_ragg->path = comp_p;
                                marc_ragg->cnt++;
                            }/* Opzione per set di esperimenti */
#ifdef TGSPN
//TGSPN
//I using an old fild
                            if (tgspn) {
                                marc_ragg->cnt = PostCond;
                            }
//TGSPN
#endif
                            c_hm += append_to_list_of_tangible_reachable(&head_marc, &tail_marc, marc_ragg, marc_ragg);

                            if (tabt[tr].tagged) {
                                throu_ragg = fill_throughput_node(tr, mean_t);
                                if (exp_set) {
                                    /* Opzione per set di esperimenti */
                                    comp_p = fill_factor(tr, ordinary_m, 1, denom_p);
                                    throu_ragg->path = comp_p;
                                    throu_ragg->cnt++;
                                }/* Opzione per set di esperimenti */
                                c_ht += append_to_list_of_throu(&head_throu, &tail_throu, throu_ragg, throu_ragg);
                            }
                        }/* Construction of Markov Chain */
//MDWN
#ifdef MDWN
                        if ((MDWN_flag) && (non_det)) {
                            get_instance(ev_p);
                            fprintf(f_MDWN, "V%lu -1 T%d %d (%s)\n", dfs_fire_ptr->marking->cont_tang, marc_ragg->cont_tang, tr, IstanceName);
                            IstanceName[0] = '\0';
                        }

#endif
//MDWN
#ifdef TGSPN
//TGSPN
                        if (tgspn) {
                            if ((tranS) && (DinstSEF) && (!PreCond) && (PostCond))
                                fprintf(f_outtgspn, "S:%lu:%f:%lu\n", marc_ragg->cont_tang, mean_t *mean, fire_ptr->marking->cont_tang);
                            if ((tranE) && (DinstSEF) && (PreCond) && (!PostCond))
                                fprintf(f_outtgspn, "E:%lu\n", marc_ragg->cont_tang);
                            if ((tranF) && (DinstSEF) && (PreCond) && (!PostCond))
                                fprintf(f_outtgspn, "F:%lu\n", marc_ragg->cont_tang);
                        }
//TGSPN
#endif

                        break;
                    case VANISHING_OLD:
#ifdef SWN
#ifdef SYMBOLIC
                        string_to_marking(reached_marking->marking->marking_as_string, reached_marking->marking->d_ptr, reached_marking->marking->length);
#endif
#endif
                        if (output_flag) {
                            /* Scrittura abilitata */
                            update_rg_files(NORMAL, NULL, UNKNOWN, 1);
                        }/* Scrittura abilitata */
//MDWN
#ifdef MDWN
                        if ((MDWN_flag) && (non_det)) {
                            get_instance(ev_p);
                            fprintf(f_MDWN, "V%lu -1 V%d %d (%s)\n", dfs_fire_ptr->marking->cont_tang, reached_marking->marking->cont_tang, tr, IstanceName);
                            IstanceName[0] = '\0';
                        }

#endif
//MDWN
                        if (out_mc) {
                            /* Construction of Markov Chain */
                            if (fast_solve) {
                                /* Opzione per soluzione veloce ma files grossi */
                                err_fseek = fseek(van_path, reached_marking->marking->path, 0);
                                load_compact(&nmark, van_path);
                                for (nk = 1; nk <= nmark; nk++) {
                                    load_compact(&st, van_path);
                                    load_double(&ra, van_path);
#ifdef TGSPN
//TGSPN
                                    if (tgspn) {
                                        load_compact(&cntTemp, van_path);
                                        if ((tranS) && (DinstSEF) && (!PreCond) && (PostCond))
                                            fprintf(f_outtgspn, "S:%d:%f:%lu\n", st, ra * mean_t *mean, fire_ptr->marking->cont_tang);
                                        if ((tranE) && (DinstSEF) && (PreCond) && (!PostCond))
                                            fprintf(f_outtgspn, "E:%d\n", st);
                                        if ((tranF) && (DinstSEF) && (PreCond) && (!PostCond))
                                            fprintf(f_outtgspn, "F:%d\n", st);
                                    }
//TGSPN
#endif
                                    marc_ragg = fill_mark_node(FALSE, st, ra * mean_t);
#ifdef TGSPN
//TGSPN
                                    if (tgspn) {
                                        marc_ragg->cnt = cntTemp;
                                    }
#endif
                                    c_hm += append_to_list_of_tangible_reachable(&head_marc, &tail_marc, marc_ragg, marc_ragg);

                                }
                            }/* Opzione per soluzione veloce ma files grossi */
                            else {
                                /* Opzione per soluzione lenta ma files contenuti */
                                marc_ragg = fill_mark_node(TRUE, reached_marking->marking->path, mean_t);
                                if (exp_set) {
                                    /* Opzione per set di esperimenti */
                                    comp_p = fill_factor(tr, ordinary_m, en_degree, denom_p);
                                    marc_ragg->path = comp_p;
                                    marc_ragg->cnt++;
                                }/* Opzione per set di esperimenti */
                                c_hm += append_to_list_of_tangible_reachable(&head_marc, &tail_marc, marc_ragg, marc_ragg);
                            }/* Opzione per soluzione lenta ma files contenuti */
                            err_fseek = fseek(rht, reached_marking->marking->throu, 0);
                            load_compact(&tnmark, rht);
                            for (tnk = 1; tnk <= tnmark; tnk++) {
                                load_compact(&tst, rht);
                                if (!exp_set) {
                                    /* Opzione per unico run */
                                    load_double(&tra, rht);
                                }/* Opzione per unico run */
                                throu_ragg = fill_throughput_node(tst, tra * mean_t);
                                if (exp_set) {
                                    /* Opzione per set di esperimenti */
                                    load_compact(&nfatt, rht);
                                    for (; nfatt ; nfatt--) {
                                        load_compact(&in_tr, rht);
                                        in_enabling_degree = 1;
#ifdef SWN
#ifdef SYMBOLIC
                                        load_compact(&in_ordinary, rht);
#endif
#endif
                                        load_compact(&in_denom_p, rht);
                                        comp_p = fill_factor(in_tr, in_ordinary, in_enabling_degree, in_denom_p);
                                        comp_p->next = throu_ragg->path;
                                        throu_ragg->path = comp_p;
                                        throu_ragg->cnt++;
                                    }
                                    comp_p = fill_factor(tr, ordinary_m, 1, denom_p);
                                    comp_p->next = throu_ragg->path;
                                    throu_ragg->path = comp_p;
                                    throu_ragg->cnt++;
                                }/* Opzione per set di esperimenti */
                                c_ht += append_to_list_of_throu(&head_throu, &tail_throu, throu_ragg, throu_ragg);
                            }
                            if (tabt[tr].tagged) {
                                throu_ragg = fill_throughput_node(tr, mean_t);
                                if (exp_set) {
                                    /* Opzione per set di esperimenti */
                                    comp_p = fill_factor(tr, ordinary_m, 1, denom_p);
                                    throu_ragg->path = comp_p;
                                    throu_ragg->cnt++;
                                }/* Opzione per set di esperimenti */
                                c_ht += append_to_list_of_throu(&head_throu, &tail_throu, throu_ragg, throu_ragg);
                            }
                        }/* Construction of Markov Chain */

                        break;
                    case TANGIBLE_OLD:
#ifdef TGSPN
//TGSPN
                        if (tgspn)
                            PostCond = checkTplaces();
//TGSPN
#endif

#ifdef SWN
#ifdef SYMBOLIC
                        string_to_marking(reached_marking->marking->marking_as_string, reached_marking->marking->d_ptr, reached_marking->marking->length);
#endif
#endif
                        if (output_flag) {
                            /* Scrittura abilitata */
                            update_rg_files(NORMAL, NULL, UNKNOWN, 0);
                        }/* Scrittura abilitata */
//MDWN
#ifdef MDWN
                        if ((MDWN_flag) && (non_det)) {
                            get_instance(ev_p);
                            fprintf(f_MDWN, "V%lu -1 T%lu %d (%s)\n", dfs_fire_ptr->marking->cont_tang, reached_marking->marking->cont_tang, tr, IstanceName);
                            IstanceName[0] = '\0';
                        }

#endif
//MDWN
#ifdef TGSPN
//TGSPN
                        if (tgspn) {
                            if ((tranS) && (DinstSEF) && (!PreCond) && (PostCond))
                                fprintf(f_outtgspn, "S:%lu:%f:%lu\n", reached_marking->marking->cont_tang, mean_t *mean, fire_ptr->marking->cont_tang);
                            if ((tranE) && (DinstSEF) && (PreCond) && (!PostCond))
                                fprintf(f_outtgspn, "E:%lu\n", reached_marking->marking->cont_tang);
                            if ((tranF) && (DinstSEF) && (PreCond) && (!PostCond))
                                fprintf(f_outtgspn, "F:%lu\n", reached_marking->marking->cont_tang);

                        }
//TGSPN
#endif
                        if (out_mc) {
                            /* Construction of Markov Chain */
                            marc_ragg = fill_mark_node(FALSE, cont_tang, mean_t);
                            if (exp_set) {
                                /* Opzione per set di esperimenti */
                                comp_p = fill_factor(tr, ordinary_m, 1, denom_p);
                                marc_ragg->path = comp_p;
                                marc_ragg->cnt++;
                            }/* Opzione per set di esperimenti */
#ifdef TGSPN
//TGSPN
                            if (tgspn) {
                                marc_ragg->cnt = PostCond;
                            }
#endif
                            c_hm += append_to_list_of_tangible_reachable(&head_marc, &tail_marc, marc_ragg, marc_ragg);

                            if (tabt[tr].tagged) {
                                throu_ragg = fill_throughput_node(tr, mean_t);
                                if (exp_set) {
                                    /* Opzione per set di esperimenti */
                                    comp_p = fill_factor(tr, ordinary_m, 1, denom_p);
                                    throu_ragg->path = comp_p;
                                    throu_ragg->cnt++;
                                }/* Opzione per set di esperimenti */
                                c_ht += append_to_list_of_throu(&head_throu, &tail_throu, throu_ragg, throu_ragg);
                            }
                        }/* Construction of Markov Chain */
                        break;
                    case VANISHING_LOOP:
                        fprintf(stdout, "error : Vanishing loop for marking:\n");
                        fprintf(stdout, "***********************************\n");
                        out_cur_marking(stdout);
                        exit(1);
                        break;
                    case NEW_MARKING:
                        enabled_head = NULL;

#ifdef SWN
#ifdef SYMBOLIC
                        string_to_marking(reached_marking->marking->marking_as_string, reached_marking->marking->d_ptr, reached_marking->marking->length);
#endif
#endif

                        update_en_list(ev_p, dfs_fire_ptr->enabled_head);
                        if (enabled_head == NULL) {
                            /* Dead marking */
                            dead++;
                            tang++;
                            check_continue_build();
                            inqueue_stack(&top, &bottom, reached_marking);
                            reached_marking->marking->cont_tang = tang;
                            reached_marking->marking->pri = -1;
                            reached_marking->enabled_head = enabled_head;
#ifdef TGSPN
//TGSPN
                            if (tgspn)
                                PostCond = checkTplaces();
//TGSPN
#endif

#ifdef SWN
#ifdef SYMBOLIC
                            ord_dead += mark_ordinarie;
#endif
#endif
                            if (output_flag)
                                update_rg_files(DEAD, NULL, UNKNOWN, UNKNOWN);

#ifdef TGSPN
//TGSPN
                            if (tgspn) {
                                if ((tranS) && (DinstSEF) && (!PreCond) && (PostCond))
                                    fprintf(f_outtgspn, "S:%lu:%f:%lu\n", reached_marking->marking->cont_tang, mean_t *mean, fire_ptr->marking->cont_tang);
                                if ((tranE) && (DinstSEF) && (PreCond) && (!PostCond))
                                    fprintf(f_outtgspn, "E:%lu\n", reached_marking->marking->cont_tang);
                                if ((tranF) && (DinstSEF) && (PreCond) && (!PostCond))
                                    fprintf(f_outtgspn, "F:%lu\n", reached_marking->marking->cont_tang);
                            }
//TGSPN
#endif

//MDWN
#ifdef MDWN
                            if ((MDWN_flag) && (non_det)) {
                                get_instance(ev_p);
                                fprintf(f_MDWN, "V%lu -1 T%lu %d (%s)\n", dfs_fire_ptr->marking->cont_tang, reached_marking->marking->cont_tang, tr, IstanceName);
                                IstanceName[0] = '\0';
                            }
#endif
//MDWN
                            if (out_mc) {
                                /* Construction of Markov Chain */
                                marc_ragg = fill_mark_node(FALSE, tang, mean_t);
                                if (exp_set) {
                                    /* Opzione per set di esperimenti */
                                    comp_p = fill_factor(tr, ordinary_m, 1, denom_p);
                                    marc_ragg->path = comp_p;
                                    marc_ragg->cnt++;
                                }/* Opzione per set di esperimenti */
#ifdef TGSPN
//TGSPN
                                if (tgspn) {
                                    marc_ragg->cnt = PostCond;
                                }
#endif
                                c_hm += append_to_list_of_tangible_reachable(&head_marc, &tail_marc, marc_ragg, marc_ragg);
                                if (tabt[tr].tagged) {
                                    throu_ragg = fill_throughput_node(tr, mean_t);
                                    if (exp_set) {
                                        /* Opzione per set di esperimenti */
                                        comp_p = fill_factor(tr, ordinary_m, 1, denom_p);
                                        throu_ragg->path = comp_p;
                                        throu_ragg->cnt++;
                                    }/* Opzione per set di esperimenti */
                                    c_ht += append_to_list_of_throu(&head_throu, &tail_throu, throu_ragg, throu_ragg);
                                }
                            }/* Construction of Markov Chain */
                        }/* Dead marking */
                        else {
                            /* Live marking */
                            reached_marking->marking->pri = cur_priority;
                            if (!cur_priority) {
                                /* Marcatura tangibile */
#ifdef MDWN
                                //	  if ((mean_t*mean>=Threshold)||(non_det))
                                //		{
#endif
                                tang++;
                                check_continue_build();
#ifdef TGSPN
//TGSPN
                                if (tgspn)
                                    PostCond = checkTplaces();

                                if (tgspn) {
                                    if ((tranS) && (DinstSEF) && (!PreCond) && (PostCond))
                                        fprintf(f_outtgspn, "S:%lu:%f:%lu\n", tang, mean_t *mean, fire_ptr->marking->cont_tang);
                                    if ((tranE) && (DinstSEF) && (PreCond) && (!PostCond))
                                        fprintf(f_outtgspn, "E:%lu\n", tang);
                                    if ((tranF) && (DinstSEF) && (PreCond) && (!PostCond))
                                        fprintf(f_outtgspn, "F:%lu\n", tang);
                                }
//TGSPN
#endif

//MDWN
//call fuction write_cvrs
#ifdef MDWN
                                if (cvrs_flag) {
                                    write_cvrs(f_cvrs, f_cvrsoff, tang);
                                }
                                if (CVRS_flag) {
                                    //if (firstmarking)
                                    fprintf(f_CVRS, "T%d: ", tang);
                                    //else
                                    //	fprintf(f_CVRS,"\tT%d: ",tang);
                                    write_on_srg(f_CVRS, 1);
                                    //firstmarking=0;
                                }
#endif
//call fuction write_cvrs
//MDWN
#ifdef SWN
#ifdef SYMBOLIC
                                ord_tang += mark_ordinarie;
#endif
#endif
                                inqueue_stack(&top, &bottom, reached_marking);
                                reached_marking->enabled_head = enabled_head;
                                reached_marking->marking->cont_tang = tang;
                                if (output_flag) {
                                    /* Scrittura abilitata */
                                    update_rg_files(NORMAL, NULL, UNKNOWN, cur_priority);
                                }/* Scrittura abilitata */

//MDWN
#ifdef MDWN
                                if ((MDWN_flag) && (non_det)) {
                                    get_instance(ev_p);
                                    fprintf(f_MDWN, "V%lu -1 T%lu %d (%s)\n", dfs_fire_ptr->marking->cont_tang, reached_marking->marking->cont_tang, tr, IstanceName);
                                    IstanceName[0] = '\0';
                                }
#endif
//MDWN
                                if (out_mc) {
                                    /* Construction of Markov Chain */
                                    marc_ragg = fill_mark_node(FALSE, tang, mean_t);
                                    if (exp_set) {
                                        /* Opzione per set di esperimenti */
                                        comp_p = fill_factor(tr, ordinary_m, 1, denom_p);
                                        marc_ragg->path = comp_p;
                                        marc_ragg->cnt++;
                                    }/* Opzione per set di esperimenti */
#ifdef TGSPN
//TGSPN
                                    if (tgspn) {
                                        marc_ragg->cnt = PostCond;
                                    }
#endif
                                    c_hm += append_to_list_of_tangible_reachable(&head_marc, &tail_marc, marc_ragg, marc_ragg);
                                    if (tabt[tr].tagged) {
                                        throu_ragg = fill_throughput_node(tr, mean_t);
                                        if (exp_set) {
                                            /* Opzione per set di esperimenti */
                                            comp_p = fill_factor(tr, ordinary_m, 1, denom_p);
                                            throu_ragg->path = comp_p;
                                            throu_ragg->cnt++;
                                        }/* Opzione per set di esperimenti */
                                        c_ht += append_to_list_of_throu(&head_throu, &tail_throu, throu_ragg, throu_ragg);
                                    }
                                }/* Construction of Markov Chain */
#ifdef MDWN
                                //}
#endif
                            }/* Marcatura tangibile */
                            else {
                                /* Marcatura evanescente */
                                evan++;
                                check_continue_build();
#ifdef SWN
#ifdef SYMBOLIC
                                ord_evan += mark_ordinarie;
#endif
#endif
//MDWN
//call fuction write_cvrs
#ifdef MDWN
//		    if(cvrs_flag)
//		    	{
//			write_cvrs(f_cvrs);
//			}
#endif
//call fuction write_cvrs
//MDWN
                                reached_marking->enabled_head = enabled_head;
                                reached_marking->marking->cont_tang = evan;
                                if (output_flag) {
                                    /* Scrittura abilitata */
                                    update_rg_files(NORMAL, NULL, UNKNOWN, cur_priority);
                                }/* Scrittura abilitata */
//MDWN
#ifdef MDWN
                                if ((MDWN_flag) && (non_det)) {
                                    get_instance(ev_p);
                                    fprintf(f_MDWN, "V%lu -1 V%lu %d (%s)\n", dfs_fire_ptr->marking->cont_tang, reached_marking->marking->cont_tang, tr, IstanceName);
                                    IstanceName[0] = '\0';
                                }
#endif
//MDWN
                                DFS_evanescenti(f_mark, mean_t *mean);
                                if (out_mc) {
                                    /* Construction of Markov Chain */
                                    pun = path_head_ptr;
                                    while (pun != NULL) {
                                        if (fast_solve) {
                                            /* Opzione per soluzione veloce ma files grossi */
                                            pun->mean_t *= mean_t;
                                        }/* Opzione per soluzione veloce ma files grossi */
                                        else {
                                            /* Opzione per soluzione lenta ma files contenuti */
                                            if (exp_set) {
                                                /* Opzione per set di esperimenti */
                                                comp_p = fill_factor(tr, ordinary_m, 1, denom_p);
                                                comp_p->next = pun->path;
                                                pun->path = comp_p;
                                                pun->cnt++;
                                            }/* Opzione per set di esperimenti */
                                            if (!exp_set) {
                                                /* Opzione per unico run */
                                                pun->mean_t *= mean_t;
                                            }/* Opzione per unico run */
                                        }/* Opzione per soluzione lenta ma files contenuti */
                                        pun = pun->next;
                                    }
                                    c_hm += append_to_list_of_tangible_reachable(&head_marc, &tail_marc, path_head_ptr, path_tail_ptr);
                                    /*c_hm += c_ph;*/
                                    path_head_ptr = NULL;
                                    path_tail_ptr = NULL;
                                    c_ph = 0;
                                    pun_throu = throu_head_ptr;
                                    while (pun_throu != NULL) {
                                        if (exp_set) {
                                            /* Opzione per set di esperimenti */
                                            comp_p = fill_factor(tr, ordinary_m, 1, denom_p);
                                            comp_p->next = pun_throu->path;
                                            pun_throu->path = comp_p;
                                            pun_throu->cnt++;
                                        }/* Opzione per set di esperimenti */
                                        if (!exp_set) {
                                            /* Opzione per unico run */
                                            pun_throu->weight *= mean_t;
                                        }/* Opzione per unico run */
                                        pun_throu = pun_throu->next;
                                    }
                                    c_ht += append_to_list_of_throu(&head_throu, &tail_throu, throu_head_ptr, throu_tail_ptr);
                                    /*c_ht += c_th;*/
                                    if (tabt[tr].tagged) {
                                        throu_ragg = fill_throughput_node(tr, mean_t);
                                        if (exp_set) {
                                            /* Opzione per set di esperimenti */
                                            comp_p = fill_factor(tr, ordinary_m, 1, denom_p);
                                            throu_ragg->path = comp_p;
                                            throu_ragg->cnt++;
                                        }/* Opzione per set di esperimenti */
                                        c_ht += append_to_list_of_throu(&head_throu, &tail_throu, throu_ragg, throu_ragg);
                                    }
                                    throu_head_ptr = NULL;
                                    throu_tail_ptr = NULL;
                                    c_th = 0;
                                }/* Construction of Markov Chain */
                            }/* Marcatura evanescente */
                        }/* Live marking */

                        break;
                    }/* Vari tipi di marcatura raggiunta */
#ifdef MDWN

                }
#endif
#ifdef GSPN
                string_to_marking(dfs_fire_ptr->marking->marking_as_string, UNKNOWN, dfs_fire_ptr->marking->length);
#endif
#ifdef SWN
#ifdef SYMBOLIC
                string_to_marking(dfs_fire_ptr->marking->marking_as_string, dfs_fire_ptr->marking->d_ptr, dfs_fire_ptr->marking->length);
#endif
#ifdef COLOURED
                string_to_marking(dfs_fire_ptr->marking->marking_as_string, UNKNOWN, dfs_fire_ptr->marking->length);
#endif
#endif

                ev_p = nev_p;
            }/* per ogni istanza */
            dispose_old_event(copy_of_ev_p);
        }/* Solo per la stessa priorita' */
        current_transition = current_transition->next;
    }/* Per tutte le transizioni abilitate */
    if (out_mc) {
        /* Construction of Markov Chain */
        if (new_reached_marking != NULL) {
            if (fast_solve) {
                new_reached_marking->marking->path = f_bot;
                err_fseek = fseek(van_path, f_bot, 0);
            }
            else
                new_reached_marking->marking->path = ftell(van_path);

            store_compact(c_hm, van_path);
            cur = head_marc;
            for (; cur != NULL ; cur = pun) {
                pun = cur->next;
                store_compact(cur->cont_tang, van_path);
                if (exp_set) {
                    /* Opzione per set di esperimenti */
                    store_compact(cur->cnt, van_path);
                    for (comp_p = cur->path; comp_p != NULL; comp_p = comp_p->next) {
                        if (cur->flag == FALSE)
                            store_compact(comp_p->fired_transition, van_path);
                        else
                            store_compact(ntr + comp_p->fired_transition, van_path);
#ifdef SWN
#ifdef SYMBOLIC
                        store_compact(comp_p->ordinary_m, van_path);
#endif
#endif
                        store_compact(comp_p->denominator, van_path);
                    }
                }/* Opzione per set di esperimenti */
                if (!exp_set) {
                    /* Opzione per unico run */
                    if (cur->flag == FALSE) {
                        store_double(&(cur->mean_t), van_path);
#ifdef TGSPN
//TGSPN
                        if (tgspn) {
                            store_compact(&(cur->cnt), van_path);
                        }
//TGSPN
#endif
                    }
                    else {
                        dval = -cur->mean_t ;
                        store_double(&dval, van_path);
                    }
                }/* Opzione per unico run */
            }
            c_ph += append_to_list_of_tangible_reachable(&path_head_ptr, &path_tail_ptr, head_marc, tail_marc);
            f_bot = ftell(van_path);
            /*c_ph += c_hm;*/

            head_marc = tail_marc = NULL;
            c_hm = 0;

            new_reached_marking->marking->throu = f_throu;
            err_fseek = fseek(rht, f_throu, 0);

            store_compact(c_ht, rht);
            cur_throu = head_throu;
            for (tnk = 1 ; tnk <= c_ht; tnk++, cur_throu = pun_throu) {
                pun_throu = cur_throu->next;
                store_compact(cur_throu->tr, rht);
                if (exp_set) {
                    /* Opzione per set di esperimenti */
                    store_compact(cur_throu->cnt, rht);
                    for (comp_p = cur_throu->path; comp_p != NULL; comp_p = comp_p->next) {
                        store_compact(comp_p->fired_transition, rht);
#ifdef SWN
#ifdef SYMBOLIC
                        store_compact(comp_p->ordinary_m, rht);
#endif
#endif
                        store_compact(comp_p->denominator, rht);
                    }
                }/* Opzione per set di esperimenti */
                if (!exp_set) {
                    /* Opzione per unico run */
                    store_double(&(cur_throu->weight), rht);
                }/* Opzione per unico run */
            }
            f_throu = ftell(rht);
            c_th += append_to_list_of_throu(&throu_head_ptr, &throu_tail_ptr, head_throu, tail_throu);
            /*c_th += c_ht;*/
            head_throu = tail_throu = NULL;
            c_ht = 0;
        }
    }/* Construction of Markov Chain */
    garbage_collect(dfs_fire_ptr);
}/* End DFS_evanescenti */
/**************************************************************/
/* NAME : */
/* DESCRIPTION : */
/* PARAMETERS : */
/* RETURN VALUE : */
/**************************************************************/
void build_graph(void) {
    /* Init build_graph */
    char cc;
    int j;
    int tr;
//MDWN
#ifdef MDWN
    time_t time_1, time_2;
    time(&time_1);
    time(&time_2);
    double Norm = 0;
#endif
//MDWN

    int marking_pri;
    int first_fire;

    Result_p current_transition;
    Result_p next_transition;

    Event_p ev_p, nev_p, copy_of_ev_p;

    Tree_p new_reached_marking;
    Throughput_p throu_ragg = NULL;
    MRate_p marc_ragg = NULL;
    PComp_p comp_p;

    double denomin = 0, numer, mean_t = 0;

    long nmark;
    int nk;
    long st, nfatt;
    unsigned long en_degree = 1;
    unsigned long ordinary_m = 1;
    double ra;

    long tnmark;
    int tnk;
    long tst;
    double tra;
    double dval;

#ifdef TGSPN
//TGSPN
    int PostCond = FALSE;
    double SumRates = 0.0;
//TGSPN
#endif

    if (out_mc) {
        /* Construction of Markov Chain */
        f_bot = ftell(van_path);
        f_throu = ftell(rht);
        fprintf(throu, "%d\n", ntr);
        fprintf(denom, "0\n");
        /*f_tang = ftell(throu);*/
    }/* Construction of Markov Chain */


    tro = 0;
    time(&old_time);
#ifdef SWN
#ifdef SYMBOLIC
    create_canonical_data_structure();
    get_canonical_marking();
#endif
#endif
    f_mark = marking_to_string();

    tro = 0;
    insert_tree(&root, &h, f_mark, length, d_ptr);
    inqueue_stack(&top, &bottom, root);
    initial_marking = root;

#ifdef GSPN
    string_to_marking(root->marking->marking_as_string, UNKNOWN, root->marking->length);
#endif
#ifdef SWN
#ifdef SYMBOLIC
    string_to_marking(root->marking->marking_as_string, root->marking->d_ptr, root->marking->length);
#endif
#ifdef COLOURED
    string_to_marking(root->marking->marking_as_string, UNKNOWN, root->marking->length);
#endif
#endif

    if (output_flag) {
        /* Scrittura abilitata */
        if (!dot_flag)
            fprintf(srg, "INITIAL MARKING\n");
        write_on_srg(srg, 0);
        if (!dot_flag)
            fprintf(srg, "\n");
    }/* Scrittura abilitata */

//MDWN
//call fuction write_cvrs
#ifdef MDWN
    if (cvrs_flag) {
        write_cvrs(f_cvrs, f_cvrsoff, 1);
    }
    if (CVRS_flag) {
        fprintf(f_CVRS, "T%1: ");
        write_on_srg(f_CVRS, 1);
        //firstmarking=0,
    }
#endif
//call fuction write_cvrs
//MDWN

    initialize_en_list();  /* Inizializzazione eventi abilitati in M0 */

    if (enabled_head != NULL) {
        /* Marcatura Live */
        root->marking->pri = cur_priority;
        if (IS_TANGIBLE(cur_priority)) {
            root->marking->cont_tang = ++tang;
#ifdef SWN
#ifdef SYMBOLIC
            ord_tang += mark_ordinarie;
#endif
#endif
        }
        else {
            root->marking->cont_tang = ++evan;
#ifdef SWN
#ifdef SYMBOLIC
            ord_evan += mark_ordinarie;
#endif
#endif
            out_error(ERROR_INITIAL_VANISHING_MARKING, 0, 0, 0, 0, NULL, NULL);
        }
    }/* Marcatura Live */
    else {
        /* Marcatura Dead */
        root->marking->pri = -1;
        root->marking->cont_tang = ++dead;
#ifdef SWN
#ifdef SYMBOLIC
        ord_dead += mark_ordinarie;
#endif
#endif
        out_error(ERROR_INITIAL_DEAD_MARKING, 0, 0, 0, 0, NULL, NULL);
    }/* Marcatura Dead */

    root->enabled_head = enabled_head;

    if (out_mc) {
        /* Construction of Markov Chain */
        for (j = 0; j < npl; j++) {
            min_place[j] = 255;
            max_place[j] = 0;
        }
        for (j = 0; j < ntr; j++)
            if (max_priority < tabt[j].pri)
                max_priority = tabt[j].pri;
        code_marking();
        arrcpy(init_place, code_place, npl);
        fprintf(wngr, "%d\n", cur_priority ? 0 : 1);
    }/* Construction of Markov Chain */

    // This is the main loop that builds the Reachability Graph
    while (top != NULL && continue_build) {
        /* per tutte le marcature memorizzate sullo stack */
        /* Khalil: The main loop to build the graph */
        if ((tang % 100000) == 0) {
            printf("T%lu V%lu\n", tang, evan);
        }
        total_throu_head_ptr = NULL;
        total_throu_tail_ptr = NULL;
        c_tth = 0;
        tangible_path_head_ptr = NULL;
        tangible_path_tail_ptr = NULL;
        c_tph = 0;
        fire_ptr = top;
        marking_pri = fire_ptr->marking->pri;
        pop(&top);
#ifdef GSPN
        string_to_marking(fire_ptr->marking->marking_as_string, UNKNOWN, fire_ptr->marking->length);
#endif
#ifdef SWN
#ifdef SYMBOLIC
        string_to_marking(fire_ptr->marking->marking_as_string, fire_ptr->marking->d_ptr, fire_ptr->marking->length);
#endif
#ifdef COLOURED
        string_to_marking(fire_ptr->marking->marking_as_string, UNKNOWN, fire_ptr->marking->length);
#endif
#endif
        if (out_mc) {
            /* Construction of Markov Chain */
            code_marking();
            write_ctrs(ctrs);
            if (IS_VANISHING(marking_pri)) {
                /* Marcatura evanascente : fattore di normalizzazione */
                denomin = normalizzazione(fire_ptr->enabled_head, marking_pri);
            }/* Marcatura evanascente : fattore di normalizzazione */
        }/* Construction of Markov Chain */
        current_transition = fire_ptr->enabled_head;
        while (current_transition != NULL && continue_build) {
            /* Per tutte le transizioni abilitate */
            next_transition = current_transition->next;
            tr = GET_TRANSITION_INDEX(current_transition->list);

#ifdef TGSPN
//TGSPN
            if (tgspn) {
                tranS = tranE = tranF = FALSE;
                checkTtrans(&tranS, &tranE, &tranF, tr);
            }
//TGSPN
#endif

            if (tabt[tr].pri == marking_pri) {
                /* Solo per transizioni alla stessa priorita' */
                /*MDWN alternanza stati*/
#ifdef MDWN
                if (strcmp(TRANS_NAME(tr), "Pr2Nd") == 0) {
                    non_det = 1;
                    //firstmarking=0;
                }
                else if (strcmp(TRANS_NAME(tr), "Nd2Pr") == 0) {
                    if (non_det == 1) {
                        time_1 = time_2;
                        time(&time_2);
                        pp_cc++;
                        printf("Step: %d, Tang:%d Van:%d  Time:%d\n", pp_cc, tang, evan, time_2 - time_1);
                    }
                    non_det = 0;
                }
#endif
                /*MDWN*/
                copy_of_ev_p = get_new_event(tr);
                ev_p = current_transition->list;
                while (ev_p != NULL && continue_build) {
                    /* Per ogni istanza abilitata */
                    nev_p = ev_p->next;

#ifdef TGSPN
//TGSPN
                    if (tgspn) {
                        if ((tranS) || (tranE) || (tranF)) {
                            //check if the transition instance contains the tagged token
                            if (strcmp(TaggedTok, "-") != 0)
                                get_instance(ev_p);
                            else
                                instSEF = TRUE; //NEUTALL
                            //check if the transition instance contains the tagged token
                            //check the precondition (Obs.  it returns true if the tagged token is in Tplace)
                            PreCond = checkTplaces();
                            //check the precondition
                        }
                    }
//TGSPN
#endif

                    if (out_mc) {
                        /* Construction of Markov Chain */
                        if (ev_p != NULL) {
                            en_degree = ev_p->enabling_degree ;
#ifdef SWN
#ifdef SYMBOLIC
                            ordinary_m = ev_p->ordinary_instances;
#endif
#endif
                        }
                        double trn_weight = get_instance_rate(ev_p);
#ifdef MDWN
                        // Change the trn_weight to account for uncertainty
                        trn_weight += tabt[ev_p->trans].uncertainty;
#endif
                        numer = en_degree *  ordinary_m * trn_weight;
                        if (IS_VANISHING(marking_pri))
                            mean_t = numer / denomin;
                        else
                            mean_t = numer;

                        update_rg_rate(mean_t);
                    }/* Construction of Markov Chain */

                    if (output_flag) {
                        /* Scrittura abilitata */
                        update_rg_files(TANGIBLE_OR_VANISHING, fire_ptr, tr, marking_pri);
                    }/* Scrittura abilitata */


                    copy_event(copy_of_ev_p, ev_p);
                    if (IS_RESET_TRANSITION(tr))
#ifdef SWN
#ifdef SYMBOLIC
                        reset_to_M0(initial_marking->marking->marking_as_string, initial_marking->marking->d_ptr, initial_marking->marking->length, tr);
#endif
#ifdef COLOURED
                    reset_to_M0(initial_marking->marking->marking_as_string, initial_marking->marking->length, tr);
#endif
#endif
#ifdef GSPN
                    reset_to_M0(initial_marking->marking->marking_as_string, initial_marking->marking->length, tr);
#endif
                    else
                        fire_trans(ev_p);
                    if (output_flag) {
                        /* Scrittura abilitata */
                        out_single_instance(ev_p, srg);
                        if (out_mc) {
                            /* Construction of Markov Chain */
                            fprintf(srg, "--->   (rate %lg)\n", mean_t);
                        }/* Construction of Markov Chain */
                    }/* Scrittura abilitata */
#ifdef SWN
#ifdef SYMBOLIC
                    get_canonical_marking();
#endif
#endif
                    f_mark = marking_to_string();
                    tro = 0;
                    insert_tree(&root, &h, f_mark, length, d_ptr); /* Piu' info nel symbolic */
                    if (current_marking == initial_marking && !home)
                        home = 1;
                    switch (tro) {
                    /* Vari tipi di marcatura raggiunta */
                    case DEAD_OLD :
                        if (out_mc) {
                            /* Construction of Markov Chain */
                            marc_ragg = fill_mark_node(FALSE, cont_tang, mean_t);
                            if (exp_set) {
                                /* Opzione per set di esperimenti */
                                comp_p = fill_factor(tr, ordinary_m, en_degree, 0);
                                marc_ragg->path = comp_p;
                                marc_ragg->cnt++;
                            }/* Opzione per set di esperimenti */
                            c_tph += append_to_list_of_tangible_reachable(&tangible_path_head_ptr, &tangible_path_tail_ptr, marc_ragg, marc_ragg);
                            if (tabt[tr].tagged) {
                                throu_ragg = fill_throughput_node(tr, mean_t);
                                if (exp_set) {
                                    /* Opzione per set di esperimenti */
                                    comp_p = fill_factor(tr, ordinary_m, en_degree, 0);
                                    throu_ragg->path = comp_p;
                                    throu_ragg->cnt++;
                                }/* Opzione per set di esperimenti */
                                c_tth += append_to_list_of_throu(&total_throu_head_ptr, &total_throu_tail_ptr, throu_ragg, throu_ragg);
                                /*MDWN*/
#ifdef MDWN
                                if (MDWN_flag) {
                                    fprintf(f_MDWN, "T%lu 1 T%d %lg\n", fire_ptr->marking->cont_tang, cont_tang, mean_t);
                                }
#endif
                                /*MDWN*/
#ifdef TGSPN
//TGSPN
                                if (tgspn) {
                                    if (((tranS) || (tranE) || (tranF)) && (instSEF)) {
                                        PostCond = checkTplaces();
                                        if ((tranS) && (instSEF) && (!PreCond) && (PostCond))
                                            fprintf(f_outtgspn, "S:%lu:%f:%lu\n", marc_ragg->cont_tang, mean_t, fire_ptr->marking->cont_tang);
                                        if ((tranE) && (instSEF) && (PreCond) && (!PostCond))
                                            fprintf(f_outtgspn, "E:%lu\n", marc_ragg->cont_tang);
                                        if ((tranF) && (instSEF) && (PreCond) && (!PostCond))
                                            fprintf(f_outtgspn, "F:%lu\n", marc_ragg->cont_tang);
                                    }

                                }
//TGSPN
#endif
                            }
                            update_rg_rate(mean_t);
                        }/* Construction of Markov Chain */
                        if (output_flag) {
                            /* Scrittura abilitata */
                            update_rg_files(NORMAL, NULL, UNKNOWN, -1);
                        }/* Scrittura abilitata */
                        break;

                    case VANISHING_OLD :
#ifdef SYMBOLIC
                        string_to_marking(reached_marking->marking->marking_as_string, reached_marking->marking->d_ptr, reached_marking->marking->length);
#endif
                        if (out_mc) {
                            /* Construction of Markov Chain */
                            if (fast_solve) {
                                /* Opzione per soluzione veloce ma files grossi */
                                err_fseek = fseek(van_path, reached_marking->marking->path, 0);
                                load_compact(&nmark, van_path);
                                /*MDWN*/
#ifdef MDWN
                                if (MDWN_flag) {
                                    if ((nmark > 0) && (!non_det)) {
                                        fprintf(f_MDWN, "T%lu %d", fire_ptr->marking->cont_tang, nmark);
                                    }
                                    else {
                                        get_instance(ev_p);
                                        fprintf(f_MDWN, "T%lu -1 V%lu %d (%s)\n", fire_ptr->marking->cont_tang, reached_marking->marking->cont_tang, tr, IstanceName);
                                        IstanceName[0] = '\0';
                                    }
                                }
#endif
                                /*MDWN*/
                                for (nk = 1; nk <= nmark; nk++) {
                                    load_compact(&st, van_path);
                                    load_double(&ra, van_path);
#ifdef TGSPN
//TGSPN
                                    if (tgspn) {
                                        load_compact(&PostCond, van_path);
                                        if ((tranS) && (instSEF) && (!PreCond) && (PostCond))
                                            fprintf(f_outtgspn, "S:%d:%f:%lu\n", st, ra * mean_t, fire_ptr->marking->cont_tang);
                                        if ((tranE) && (instSEF) && (PreCond) && (!PostCond))
                                            fprintf(f_outtgspn, "E:%d\n", st);
                                        if ((tranF) && (instSEF) && (PreCond) && (!PostCond))
                                            fprintf(f_outtgspn, "F:%d\n", st);

                                    }

//TGSPN
#endif
                                    marc_ragg = fill_mark_node(FALSE, st, ra * mean_t);
                                    /*MDWN*/
#ifdef MDWN
                                    if ((MDWN_flag) && (!non_det)) {
                                        fprintf(f_MDWN, " T%lu %lg", marc_ragg->cont_tang, ra * mean_t);
                                    }
#endif
                                    /*MDWN*/
                                    c_tph += append_to_list_of_tangible_reachable(&tangible_path_head_ptr, &tangible_path_tail_ptr, marc_ragg, marc_ragg);
                                }
                                /*MDWN*/
#ifdef MDWN
                                if ((MDWN_flag) && (nmark > 0) && (!non_det)) {
                                    fprintf(f_MDWN, "\n");
                                }

#endif
                                /*MDWN*/
                            }/* Opzione per soluzione veloce ma files grossi */
                            else {
                                /* Opzione per soluzione lenta ma files contenuti */
                                marc_ragg = fill_mark_node(TRUE, reached_marking->marking->path, -mean_t);
                                if (exp_set) {
                                    /* Opzione per set di esperimenti */
                                    comp_p = fill_factor(tr, ordinary_m, en_degree, 0);
                                    marc_ragg->path = comp_p;
                                    marc_ragg->cnt++;
                                }/* Opzione per set di esperimenti */
                                c_tph += append_to_list_of_tangible_reachable(&tangible_path_head_ptr, &tangible_path_tail_ptr, marc_ragg, marc_ragg);
                            }/* Opzione per soluzione lenta ma files contenuti */
                            err_fseek = fseek(rht, reached_marking->marking->throu, 0);
                            load_compact(&tnmark, rht);
                            for (tnk = 1; tnk <= tnmark; tnk++) {
                                load_compact(&tst, rht);
                                if (!exp_set) {
                                    /* Opzione per unico run */
                                    load_double(&tra, rht);
                                }/* Opzione per unico run */
                                throu_ragg = fill_throughput_node(tst, tra * mean_t);
                                if (exp_set) {
                                    /* Opzione per set di esperimenti */
                                    load_compact(&nfatt, rht);
                                    for (; nfatt ; nfatt--) {
                                        load_compact(&in_tr, rht);
                                        in_enabling_degree = 1;
#ifdef SWN
#ifdef SYMBOLIC
                                        load_compact(&in_ordinary, rht);
#endif
#endif
                                        load_compact(&in_denom_p, rht);
                                        comp_p = fill_factor(in_tr, in_ordinary, in_enabling_degree, in_denom_p);
                                        comp_p->next = throu_ragg->path;
                                        throu_ragg->path = comp_p;
                                        throu_ragg->cnt++;
                                    }
                                    comp_p = fill_factor(tr, ordinary_m, en_degree, 0);
                                    comp_p->next = throu_ragg->path;
                                    throu_ragg->path = comp_p;
                                    throu_ragg->cnt++;
                                }/* Opzione per set di esperimenti */
                                c_tth += append_to_list_of_throu(&total_throu_head_ptr, &total_throu_tail_ptr, throu_ragg, throu_ragg);
                            }
                            if (tabt[tr].tagged) {
                                throu_ragg = fill_throughput_node(tr, mean_t);
                                if (exp_set) {
                                    /* Opzione per set di esperimenti */
                                    comp_p = fill_factor(tr, ordinary_m, en_degree, 0);
                                    throu_ragg->path = comp_p;
                                    throu_ragg->cnt++;
                                }/* Opzione per set di esperimenti */
                                c_tth += append_to_list_of_throu(&total_throu_head_ptr, &total_throu_tail_ptr, throu_ragg, throu_ragg);
                            }
                            update_rg_rate(mean_t);
                        }/* Construction of Markov Chain */

                        if (output_flag) {
                            /* Scrittura abilitata */
                            update_rg_files(NORMAL, NULL, UNKNOWN, 1);
                        }/* Scrittura abilitata */
                        break;

                    case TANGIBLE_OLD :
#ifdef SYMBOLIC
                        string_to_marking(reached_marking->marking->marking_as_string, reached_marking->marking->d_ptr, reached_marking->marking->length);
#endif
                        if (out_mc) {
                            /* Construction of Markov Chain */
                            marc_ragg = fill_mark_node(FALSE, cont_tang, mean_t);
                            /*MDWN*/
#ifdef MDWN
                            if (MDWN_flag) {
                                fprintf(f_MDWN, "T%lu 1 T%lu %lg\n", fire_ptr->marking->cont_tang, marc_ragg->cont_tang, mean_t);
                            }
#endif
                            /*MDWN*/
#ifdef TGSPN
//TGSPN
                            if (tgspn) {
                                if (((tranS) || (tranE) || (tranF)) && (instSEF)) {
                                    PostCond = checkTplaces();
                                    if ((tranS) && (instSEF) && (!PreCond) && (PostCond))
                                        fprintf(f_outtgspn, "S:%lu:%f:%lu\n", reached_marking->marking->cont_tang, mean_t, fire_ptr->marking->cont_tang);
                                    if ((tranE) && (instSEF) && (PreCond) && (!PostCond))
                                        fprintf(f_outtgspn, "E:%lu\n", reached_marking->marking->cont_tang);
                                    if ((tranF) && (instSEF) && (PreCond) && (!PostCond))
                                        fprintf(f_outtgspn, "F:%lu\n", reached_marking->marking->cont_tang);
                                }

                            }
//TGSPN
#endif
                            if (exp_set) {
                                /* Opzione per set di esperimenti */
                                comp_p = fill_factor(tr, ordinary_m, en_degree, 0);
                                marc_ragg->path = comp_p;
                                marc_ragg->cnt++;
                            }/* Opzione per set di esperimenti */

                            c_tph += append_to_list_of_tangible_reachable(&tangible_path_head_ptr, &tangible_path_tail_ptr, marc_ragg, marc_ragg);


                            if (tabt[tr].tagged) {
                                throu_ragg = fill_throughput_node(tr, mean_t);
                                if (exp_set) {
                                    /* Opzione per set di esperimenti */
                                    comp_p = fill_factor(tr, ordinary_m, en_degree, 0);
                                    throu_ragg->path = comp_p;
                                    throu_ragg->cnt++;
                                }/* Opzione per set di esperimenti */
                                c_tth += append_to_list_of_throu(&total_throu_head_ptr, &total_throu_tail_ptr, throu_ragg, throu_ragg);
                            }
                            update_rg_rate(mean_t);
                        }/* Construction of Markov Chain */

                        if (output_flag) {
                            /* Scrittura abilitata */
                            update_rg_files(NORMAL, NULL, UNKNOWN, 0);
                        }/* Scrittura abilitata */
                        break;
                    case NEW_MARKING :
                        enabled_head = NULL;
#ifdef SWN
#ifdef SYMBOLIC
                        string_to_marking(reached_marking->marking->marking_as_string, reached_marking->marking->d_ptr, reached_marking->marking->length);
#endif
#endif
                        update_en_list(ev_p, fire_ptr->enabled_head);
                        if (enabled_head == NULL) {
                            /* Dead marking */
                            tang++;
                            dead++;
                            check_continue_build();

                            inqueue_stack(&top, &bottom, reached_marking);
                            reached_marking->marking->cont_tang = tang;
                            reached_marking->marking->pri = -1;
                            reached_marking->enabled_head = enabled_head;
#ifdef SWN
#ifdef SYMBOLIC
                            ord_dead += mark_ordinarie;
#endif
#endif
#ifdef TGSPN
//TGSPN
                            if (tgspn) {
                                if (((tranS) || (tranE) || (tranF)) && (instSEF)) {
                                    PostCond = checkTplaces();
                                    if ((tranS) && (instSEF) && (!PreCond) && (PostCond))
                                        fprintf(f_outtgspn, "S:%lu:%f:%lu\n", tang, mean_t, fire_ptr->marking->cont_tang);
                                    if ((tranE) && (instSEF) && (PreCond) && (!PostCond))
                                        fprintf(f_outtgspn, "E:%lu\n", tang);
                                    if ((tranF) && (instSEF) && (PreCond) && (!PostCond))
                                        fprintf(f_outtgspn, "F:%lu\n", tang);
                                }

                            }
//TGSPN
#endif
                            if (out_mc) {
                                /* Construction of Markov Chain */
                                marc_ragg = fill_mark_node(FALSE, tang, mean_t);
                                /*MDWN*/
#ifdef MDWN
                                if (MDWN_flag) {
                                    fprintf(f_MDWN, "T%lu 1 T%lu %lg\n", fire_ptr->marking->cont_tang, marc_ragg->cont_tang, mean_t);
                                }
#endif
                                /*MDWN*/
                                if (exp_set) {
                                    /* Opzione per set di esperimenti */
                                    comp_p = fill_factor(tr, ordinary_m, en_degree, 0);
                                    marc_ragg->path = comp_p;
                                    marc_ragg->cnt++;
                                }/* Opzione per set di esperimenti */

                                c_tph += append_to_list_of_tangible_reachable(&tangible_path_head_ptr, &tangible_path_tail_ptr, marc_ragg, marc_ragg);

                                if (tabt[tr].tagged) {
                                    throu_ragg = fill_throughput_node(tr, mean_t);
                                    if (exp_set) {
                                        /* Opzione per set di esperimenti */
                                        comp_p = fill_factor(tr, ordinary_m, en_degree, 0);
                                        throu_ragg->path = comp_p;
                                        throu_ragg->cnt++;
                                    }/* Opzione per set di esperimenti */
                                    c_tth += append_to_list_of_throu(&total_throu_head_ptr, &total_throu_tail_ptr, throu_ragg, throu_ragg);
                                }
                                update_rg_rate(mean_t);
                            }/* Construction of Markov Chain */

                            if (output_flag)
                                update_rg_files(DEAD, NULL, UNKNOWN, UNKNOWN);
                        }/* Dead marking */
                        else {
                            /* Live Marking */
                            reached_marking->marking->pri = cur_priority;
                            if (!cur_priority) {
                                /* marcatura tangibile */
                                tang++;
                                check_continue_build();
#ifdef SWN
#ifdef SYMBOLIC
                                ord_tang += mark_ordinarie;
#endif
#endif

#ifdef TGSPN
//TGSPN
                                if (tgspn) {
                                    if (((tranS) || (tranE) || (tranF)) && (instSEF)) {
                                        PostCond = checkTplaces();
                                        if ((tranS) && (instSEF) && (!PreCond) && (PostCond))
                                            fprintf(f_outtgspn, "S:%lu:%f:%lu\n", tang, mean_t, fire_ptr->marking->cont_tang);
                                        if ((tranE) && (instSEF) && (PreCond) && (!PostCond))
                                            fprintf(f_outtgspn, "E:%lu\n", tang);
                                        if ((tranF) && (instSEF) && (PreCond) && (!PostCond))
                                            fprintf(f_outtgspn, "F:%lu\n", tang);
                                    }

                                }
//TGSPN
#endif
                                inqueue_stack(&top, &bottom, reached_marking);
                                reached_marking->enabled_head = enabled_head;
                                reached_marking->marking->cont_tang = tang;
                                
                                if (out_mc) {
                                    /* Construction of Markov Chain */
                                    marc_ragg = fill_mark_node(FALSE, tang, mean_t);
                                    /*MDWN*/
#ifdef MDWN
                                    if (MDWN_flag) {
                                        fprintf(f_MDWN, "T%lu 1 T%lu %lg\n", fire_ptr->marking->cont_tang, marc_ragg->cont_tang, mean_t);
                                    }
#endif
                                    /*MDWN*/
                                    if (exp_set) {
                                        /* Opzione per set di esperimenti */
                                        comp_p = fill_factor(tr, ordinary_m, en_degree, 0);
                                        marc_ragg->path = comp_p;
                                        marc_ragg->cnt++;
                                    }/* Opzione per set di esperimenti */

                                    c_tph += append_to_list_of_tangible_reachable(&tangible_path_head_ptr, &tangible_path_tail_ptr, marc_ragg, marc_ragg);

                                    if (tabt[tr].tagged) {
                                        throu_ragg = fill_throughput_node(tr, mean_t);
                                        if (exp_set) {
                                            /* Opzione per set di esperimenti */
                                            comp_p = fill_factor(tr, ordinary_m, en_degree, 0);
                                            throu_ragg->path = comp_p;
                                            throu_ragg->cnt++;
                                        }/* Opzione per set di esperimenti */
                                        c_tth += append_to_list_of_throu(&total_throu_head_ptr, &total_throu_tail_ptr, throu_ragg, throu_ragg);
                                    }
                                    update_rg_rate(mean_t);
                                }/* Construction of Markov Chain */

                                if (output_flag) {
                                    /* Scrittura abilitata */
                                    update_rg_files(NORMAL, NULL, UNKNOWN, cur_priority);
                                }/* Scrittura abilitata */
                            }/* marcatura tangibile */
                            else {
                                /* marcatura evanescente */
                                evan++;
                                check_continue_build();
#ifdef SWN
#ifdef SYMBOLIC
                                ord_evan += mark_ordinarie;
#endif
#endif
                                new_reached_marking = reached_marking;
                                reached_marking->enabled_head = enabled_head;
                                reached_marking->marking->cont_tang = evan;
                                if (out_mc) {
                                    /* Construction of Markov Chain */
                                    path_head_ptr = NULL;
                                    path_tail_ptr = NULL;
                                    c_ph = 0;
                                    throu_head_ptr = NULL;
                                    throu_tail_ptr = NULL;
                                    c_th = 0;
                                    update_rg_rate(mean_t);
                                }/* Construction of Markov Chain */

                                if (output_flag) {
                                    /* Scrittura abilitata */
                                    update_rg_files(NORMAL, NULL, UNKNOWN, cur_priority);
                                }/* Scrittura abilitata */
                                /*MDWN*/
#ifdef MDWN
                                if ((MDWN_flag) && (non_det)) {
                                    get_instance(ev_p);
                                    fprintf(f_MDWN, "T%lu -1 V%lu %d (%s) \n", fire_ptr->marking->cont_tang, reached_marking->marking->cont_tang, tr, IstanceName);
                                    IstanceName[0] = '\0';
                                }

#endif
                                /*MDWN*/
                                DFS_evanescenti(f_mark, mean_t);
                                if (out_mc) {
                                    /* Construction of Markov Chain */
                                    if (fast_solve) {
                                        /* Opzione per soluzione veloce ma files grossi */
                                        err_fseek = fseek(van_path, new_reached_marking->marking->path, 0);
                                        load_compact(&nmark, van_path);
                                        /*MDWN*/
#ifdef MDWN
                                        if (MDWN_flag) {
                                            if ((nmark > 0) && (!non_det)) {
                                                fprintf(f_MDWN, "T%lu %d", fire_ptr->marking->cont_tang, nmark);
                                            }
                                            //else
                                            //  if (non_det)
                                            //	{
                                            //	fprintf(f_MDWN,"T%d -1 V%d %d",fire_ptr->marking->cont_tang,marc_ragg->cont_tang,tr);
                                            //	}
                                        }
#endif
                                        /*MDWN*/
                                        /*MDWN*/
#ifdef MDWN
                                        Norm = 0.0;
                                        for (nk = 1; nk <= nmark; nk++) {
                                            load_compact(&st, van_path);
                                            load_double(&ra, van_path);
                                            Norm = Norm + ra * mean_t;
                                        }
                                        static int warn_if_norm = 0;
                                        if (Norm != 1.0 && !warn_if_norm) {
                                            printf("NOTE: Normalization value is not 1.0 and normalization "
                                                   "has been disabled for uncertain MDPN.\n");
                                            warn_if_norm = 1;
                                        }
                                        Norm = 1.0;
                                        // printf("Norm = %lf\n", Norm);
                                        /*
                                        if ((MDWN_flag)&&(!non_det))
                                        		{
                                         		printf("%f\n",Norm);
                                        		}
                                         */
                                        err_fseek = fseek(van_path, new_reached_marking->marking->path, 0);
                                        load_compact(&nmark, van_path);
#endif
                                        /*MDWN*/
                                        for (nk = 1; nk <= nmark; nk++) {
                                            load_compact(&st, van_path);
                                            load_double(&ra, van_path);
#ifdef TGSPN
                                            if (tgspn) {
                                                load_compact(&PostCond, van_path);
                                            }
#endif
                                            marc_ragg = fill_mark_node(FALSE, st, ra * mean_t);
                                            /*MDWN*/
#ifdef MDWN
                                            if ((MDWN_flag) && (!non_det)) {
                                                fprintf(f_MDWN, " T%lu %lg", marc_ragg->cont_tang, ra * mean_t / Norm);
                                            }
                                            /*if ((MDWN_flag)&&(!non_det))
                                            	{
                                             	printf("\t%f\n",ra*mean_t);
                                            	}
                                            */
#endif
                                            /*MDWN*/

                                            c_tph += append_to_list_of_tangible_reachable(&tangible_path_head_ptr, &tangible_path_tail_ptr, marc_ragg, marc_ragg);
                                        }
                                        /*MDWN*/
#ifdef MDWN
                                        if ((MDWN_flag) && (nmark > 0) && (!non_det)) {
                                            fprintf(f_MDWN, "\n");
                                        }
#endif
                                        /*MDWN*/
                                    }/* Opzione per soluzione veloce ma files grossi */
                                    else {
                                        /* Opzione per soluzione lenta ma files contenuti */
                                        marc_ragg = fill_mark_node(TRUE, new_reached_marking->marking->path, -mean_t);
                                        if (exp_set) {
                                            /* Opzione per set di esperimenti */
                                            comp_p = fill_factor(tr, ordinary_m, en_degree, 0);
                                            marc_ragg->path = comp_p;
                                            marc_ragg->cnt++;
                                        }/* Opzione per set di esperimenti */
                                        c_tph += append_to_list_of_tangible_reachable(&tangible_path_head_ptr, &tangible_path_tail_ptr, marc_ragg, marc_ragg);
                                    }/* Opzione per soluzione lenta ma files contenuti */
                                    push_rate_list(path_head_ptr, path_tail_ptr);
                                    for (throu_ragg =  throu_head_ptr; throu_ragg != NULL; throu_ragg = throu_ragg->next) {
                                        if (exp_set) {
                                            /* Opzione per set di esperimenti */
                                            comp_p = fill_factor(tr, ordinary_m, en_degree, 0);
                                            comp_p->next = throu_ragg->path;
                                            throu_ragg->path = comp_p;
                                            throu_ragg->cnt++;
                                        }/* Opzione per set di esperimenti */
                                        if (!exp_set) {
                                            /* Opzione per unico run */
                                            throu_ragg->weight *= mean_t;
                                        }/* Opzione per unico run */
                                    }
                                    c_tth += append_to_list_of_throu(&total_throu_head_ptr, &total_throu_tail_ptr, throu_head_ptr, throu_tail_ptr);
                                    /*c_tth += c_th;*/
                                    if (tabt[tr].tagged) {
                                        throu_ragg = fill_throughput_node(tr, mean_t);
                                        if (exp_set) {
                                            /* Opzione per set di esperimenti */
                                            comp_p = fill_factor(tr, ordinary_m, en_degree, 0);
                                            throu_ragg->path = comp_p;
                                            throu_ragg->cnt++;
                                        }/* Opzione per set di esperimenti */
                                        c_tth += append_to_list_of_throu(&total_throu_head_ptr, &total_throu_tail_ptr, throu_ragg, throu_ragg);
                                    }
                                }/* Construction of Markov Chain */
                            }/* Marcatura evanescente */
                        }/* Live Marking */

                        break;
                    }/* Vari tipi di marcatura raggiunta */
#ifdef GSPN
                    string_to_marking(fire_ptr->marking->marking_as_string, UNKNOWN, fire_ptr->marking->length);
#endif
#ifdef SWN
#ifdef SYMBOLIC
                    string_to_marking(fire_ptr->marking->marking_as_string, fire_ptr->marking->d_ptr, fire_ptr->marking->length);
#endif
#ifdef COLOURED
                    string_to_marking(fire_ptr->marking->marking_as_string, UNKNOWN, fire_ptr->marking->length);
#endif
#endif
                    ev_p = nev_p;
                }/* Per ogni istanza abilitata */
                dispose_old_event(copy_of_ev_p);
            }/* Solo per transizioni alla stessa priorita' */
            current_transition = next_transition;
        }/* Per tutte le transizioni abilitate */

        if (out_mc) {
            /* Construction of Markov Chain */
#if TGSPN
            //fprintf(wngr,"%d",tang);
            if (tgspn) {
                SumRates = 0.0;
                fprintf(wngr, "%ld\n", c_tph);
            }
            else
                store_compact(c_tph, wngr);
#else
            store_compact(c_tph, wngr);

#endif
            marc_ragg = tangible_path_head_ptr;
            for (; marc_ragg != NULL ; marc_ragg = marc_ragg->next) {

#if TGSPN
                if (tgspn)
                    fprintf(wngr, "\t%lu", marc_ragg->cont_tang);
                else
                    store_compact(marc_ragg->cont_tang, wngr);
#else
                store_compact(marc_ragg->cont_tang, wngr);
#endif
                if (exp_set) {
                    /* Opzione per set di esperimenti */
                    if (marc_ragg->flag == TRUE)
                        store_compact(ntr + marc_ragg->path->fired_transition, wngr);
                    else
                        store_compact(marc_ragg->path->fired_transition, wngr);
                    store_compact(marc_ragg->path->enabling_degree, wngr);
#ifdef SWN
#ifdef SYMBOLIC
                    store_compact(marc_ragg->path->ordinary_m, wngr);
#endif
#endif
                }/* Opzione per set di esperimenti */
                if (!exp_set) {
                    /* Opzione per unico run */
#if TGSPN
                    if (tgspn) {
                        fprintf(wngr, " %f\n", marc_ragg->mean_t);
                        SumRates += marc_ragg->mean_t;
                    }
                    else {
                        store_double(&(marc_ragg->mean_t), wngr);
                    }
#else
                    store_double(&(marc_ragg->mean_t), wngr);
#endif
                }/* Opzione per unico run */
            }
#if TGSPN
            //fprintf(wngr,"%f\n",-1*SumRates);
#endif
            store_compact(c_tth, throu);
            throu_ragg = total_throu_head_ptr;
            for (tnk = 1 ; tnk <= c_tth; tnk++, throu_ragg = throu_ragg->next) {
                store_compact(throu_ragg->tr, throu);
                if (exp_set) {
                    /* Opzione per set di esperimenti */
                    store_compact(throu_ragg->cnt, throu);
                    for (comp_p = throu_ragg->path; comp_p != NULL; comp_p = comp_p->next) {
                        store_compact(comp_p->fired_transition, throu);
                        if (! IS_IMMEDIATE(comp_p->fired_transition))
                            store_compact(comp_p->enabling_degree, throu);
#ifdef SWN
#ifdef SYMBOLIC
                        store_compact(comp_p->ordinary_m, throu);
#endif
#endif
                        store_compact(comp_p->denominator, throu);
                    }
                }/* Opzione per set di esperimenti */
                if (!exp_set) {
                    /* Opzione per unico run */
                    store_double(&(throu_ragg->weight), throu);
                }/* Opzione per unico run */
            }
            push_throu_list(total_throu_head_ptr, total_throu_tail_ptr);
            push_rate_list(tangible_path_head_ptr, tangible_path_tail_ptr);
        }/* Construction of Markov Chain */
        garbage_collect(fire_ptr);
    }/* per tutte le marcature memorizzate sullo stack */
    time(&new_time);
    if (out_mc) {
        /* Construction of Markov Chain */
        write_grg(grg);
        fprintf(wngr, "%ld\n", tang + 1);
        fprintf(rgr_aux, "toptan= %ld\n", tang);
        fprintf(rgr_aux, "topvan= %ld\n", evan);
        fprintf(throu, "\n");
        for (st = 0; st < ntr; st++)
            fprintf(throu, "%s\n", TRANS_NAME(st));
    }/* Construction of Markov Chain */
    if (output_flag) {
        /* Scrittura abilitata */
        write_final_results(srg);
    }/* Scrittura abilitata */
    write_final_results(stdout);
    if (dead > 0)
        fprintf(stdout, "Reachability graph contains dead markings !!\n");
}/* End build_graph */
#endif






/************************* API FUNCTIONS FOR SPOT *******************************/
#ifdef LIBSPOT

#include "../../INCLUDE/gspnlib.h"

/* ------------------ State Space Exploration -----------------*/


static void set_state(const State s, int force) {
    static State current = NULL;
    if (current != s || force) {
#ifdef SWN
#ifdef SYMBOLIC
        string_to_marking(s->marking->marking_as_string, s->marking->d_ptr, s->marking->length);
#endif
#ifdef COLOURED
        string_to_marking(s->marking->marking_as_string, UNKNOWN, s->marking->length);
#endif
#endif
    }
    current = s;
}



/* Returns the identifier of the initial marking state */
extern int initial_state(pState M0) {

    /* first time called */
    if (! initial_marking) {
#ifdef SWN
#ifdef SYMBOLIC
        create_canonical_data_structure();
        get_canonical_marking();
#endif
#endif
        f_mark = marking_to_string();

        tro = 0;
        insert_tree(&root, &h, f_mark, length, d_ptr);
        initial_marking = root;
    }

    /* initial marking is initialized */

    *M0 = initial_marking ;

    return 0;
}


/* Prints the state "s" specified to "fd" */
int print_state(const State s, char **st) {

    FILE *fd;
    int pos;
    char  *mark = "states.mark";
    fd = fopen(mark, "w+");
    set_state(s, 0);
    write_on_srg(fd, (s == initial_marking ? 0 : 1));
    pos = ftell(fd);
    *st = malloc((pos + 1) * sizeof(char));
    fseek(fd, 0, SEEK_SET);
    fread(*st, sizeof(char), pos * sizeof(char), fd);
    (*st)[pos] = '\0';
    fclose(fd);
    return 0;

}


/* Given a state "s" and a list of "props_size" property indexes "props" checks the truth value of
   every atomic property in "props" and returns in "truth" the truth value of these properties
   in the same order as the input, ONE CHAR PER TRUTH VALUE (i.e. sizeof(truth[]) = props_size   NB : the vector "truth" is allocated in this function
*/
extern int satisfy(const State s, const AtomicProp  props [],  unsigned char **truth, size_t props_size) {
    /*  sizeof(unsigned char) = 1 */
    (*truth) = malloc(props_size) ;

    set_state(s, 0);
    /* suppose all props are transitions */
    int i;
    for (i = 0 ; i < props_size ; i++) {
        /* if (i < npropTrans ) */
        (*truth)[i] = is_enabled(props[i]);
    }


    return 0;

}


/* free the "truth" vector allocated by satisfy
   !!! NB: Don't forget to call me, or you will get a memory leak !!!
*/
extern int satisfy_free(unsigned char *truth) {
    free(truth);
    return 0;
}


/* Calculates successors of a state "s"

   Each successor is returned in a struct that gives the Event property verified by the transition
   fired to reach this marking;
   If a marking is reached by firing a transition observed by more than one event property, it will
   be returned in many copies:
   i.e. E1 and E2 observe different firngs of transition t1 ; M1 is reached from M0 by firing t1 with
   a binding observable by both E1 and E2 :
   succ (M0, [E1,E2] , ...)
   will return {[M1,E1],[M1,E2]}


   NB : "succ" vector is allocated in the function, use succ_free for memory release
*/
extern int succ(const State s,
                EventPropSucc **succ, size_t *succ_size) {

    int tr, marking_pri;

    Result_p current_transition;
    Result_p next_transition;
    Event_p ev_p, nev_p, copy_of_ev_p;

    *succ = NULL;
    *succ_size = 0;
    /*  succ = malloc (sizeof(EventPropSucc)* (*succ_size)); */


    /* Set current marking to argument "s" */
    set_state(s, 0);

    /* calculate list of enabled transitions */
    /* Also calculates cur_priority for current marking i.e. highest priority of enabled trans. */
    enabled_head = NULL;
    my_initialize_en_list();

    current_transition = enabled_head;
    s->enabled_head = enabled_head;
    /* iterate over all enabled transitions */
    while (current_transition != NULL) {
        next_transition = current_transition->next;
        tr = GET_TRANSITION_INDEX(current_transition->list);

        int i;
        AtomicProp eprop = EVENT_TRUE;
        for (i = 0; i < tab_event_prop_size ; i++) {
            if (tr == tab_event_prop[i]) {
                eprop = tr;
                break;
            }
        }

        /* only for transistions with current/highest priority*/
        if (tabt[tr].pri == cur_priority) {
            copy_of_ev_p = get_new_event(tr);
            ev_p = current_transition->list;

            /* for all enabled instances  */
            while (ev_p != NULL) {
                nev_p = ev_p->next;
                copy_event(copy_of_ev_p, ev_p);
                fire_trans(ev_p);

#ifdef SWN
#ifdef SYMBOLIC
                get_canonical_marking();
#endif
#endif
                f_mark = marking_to_string();
                tro = 0;
                insert_tree(&root, &h, f_mark, length, d_ptr);


                /* Add this new state to list of successors */
                (*succ_size)++ ;
                *succ = realloc(*succ, sizeof(EventPropSucc) * (*succ_size));
                (*succ + (*succ_size) - 1)->s = reached_marking ;
                (*succ + (*succ_size) - 1)->p = eprop ;

                if (current_marking == initial_marking && !home) home = 1;

                set_state(s, 1);


                /* next instance for current transition */
                ev_p = nev_p;

            }/* for all enabled instances  */

            dispose_old_event(copy_of_ev_p);
        }/* only for transistions with highest priority */

        current_transition = next_transition;
    }/* iterate over all enabled transitions */

    garbage_collect(s);

    return 0;
}

/* free the "succ" vector allocated by succ
   !!! NB: Don't forget to call me, or you will get a memory leak !!!
*/
extern int succ_free(EventPropSucc *succ) {
    free(succ);

    return 0;
}


#endif




/************************* API FUNCTIONS FOR SPOT *******************************/
#ifdef LIBDMC

#include "../../INCLUDE/gspndmclib.h"

/* ------------------ State Space Exploration -----------------*/
extern char  cache_string[];

int dmcInitialize(int argc, char **argv) {
    initialize(argc, argv);
#ifdef SWN
#ifdef SYMBOLIC
    create_canonical_data_structure();
#endif
#endif
    // to load marking from strings without mark file (thx Soheib)
    SPEC_TRAI = TRUE;
    return 0;
}


/// Allocate a new dmcState structure (constructor)
pdmcState dmcStateNew(char *data, size_t d_ptr, size_t length) {
    pdmcState ret = malloc(sizeof(dmcState));
    if (length) {
        ret->data = malloc(length);
        memcpy(ret->data, data, length);
    }
    else {
        ret->data = NULL;
    }
    ret->d_ptr = d_ptr;
    ret->length = length;
    return ret;
}

/// deep copy of a dmcState
pdmcState dmcStateClone(const pdmcState s) {
    return  dmcStateNew(s->data, s->d_ptr, s->length);
}

/// deallocate a dmcState
void dmcStateFree(pdmcState s) {
    free(s->data);
    free(s);
}

/// compare two dmcState
int dmcStateEq(const pdmcState a, const pdmcState b) {
    return (a->length == b->length) && (a->d_ptr == b->d_ptr) && (! memcmp(a->data, b->data, a->length));
}

/// set current state
pdmcState current = NULL;
extern void dmc_set_state(const pdmcState s, int force) {
    if (!current) current = dmcStateNew(NULL, 0, 0);
    if (! dmcStateEq(current, s) || force) {
        memcpy(cache_string, s->data, s->length);
#ifdef SWN
#ifdef SYMBOLIC
        string_to_marking(0, s->d_ptr, s->length);
#endif
#ifdef COLOURED
        string_to_marking(0, UNKNOWN, s->length);
#endif
#endif
        dmcStateFree(current);
        current = dmcStateClone(s);
    }
}


/* Returns the identifier of the initial marking state */
pdmcState initial_dmcState = NULL;
extern int dmc_initial_state(pdmcState *M0) {

    /* first time called */
    if (! initial_dmcState) {
        SPEC_TRAI = TRUE;
#ifdef SWN
#ifdef SYMBOLIC
        get_canonical_marking();
#endif
#endif
        marking_to_string();
        initial_dmcState = dmcStateNew(cache_string, d_ptr, length);
    }

    /* initial marking is initialized */
    *M0 = dmcStateClone(initial_dmcState);
    return 0;
}


/* Prints the state "s" specified to "fd" */
int dmc_print_state(const pdmcState s, char **st) {

    FILE *fd;
    int pos;
    char  *mark = "states.mark";
    fd = fopen(mark, "w+");
    dmc_set_state(s, 0);
    write_on_srg(fd, (dmcStateEq(s, initial_dmcState) ? 0 : 1));
    pos = ftell(fd);
    *st = malloc((pos + 1) * sizeof(char));
    fseek(fd, 0, SEEK_SET);
    fread(*st, sizeof(char), pos * sizeof(char), fd);
    (*st)[pos] = '\0';
    fclose(fd);
    return 0;

}

/** returns the number of concrete states associated to a symbolic state */
long dmc_concrete_state_count(const pdmcState s) {
    dmc_set_state(s, 0);
#ifdef SWN
#ifdef SYMBOLIC

    /*   char *str; */
    /*   dmc_print_state (s,&str); */
    /*   fprintf(stderr,"OK : %s\n",str); */
    /*   free(str); */
    get_canonical_marking();
    (void) marking_to_string();
    dmc_set_state(s, 1);
    return mark_ordinarie;
//  return current_marking->marking->ordinary ;
#else
    return 1;
#endif
#endif
}

/*** To free event structures after firing events over a state */
static void dmc_garbage_collect(Result_p enabled) {
    /* Init garbage_collect */
    Result_p current_transition, next_transition;

    current_transition = enabled;
    while (current_transition != NULL) {
        next_transition = current_transition->next;
        dispose_all_instances(current_transition);
        current_transition->list = NULL;
        push_result(current_transition);
        current_transition = next_transition;
    }

}/* End garbage_collect */


/* Calculates successors of a state "s"

   Each successor is returned in a struct that gives the Event property verified by the transition
   fired to reach this marking;
   If a marking is reached by firing a transition observed by more than one event property, it will
   be returned in many copies:
   i.e. E1 and E2 observe different firngs of transition t1 ; M1 is reached from M0 by firing t1 with
   a binding observable by both E1 and E2 :
   succ (M0, [E1,E2] , ...)
   will return {[M1,E1],[M1,E2]}


   NB : "succ" vector is allocated in the function, use succ_free for memory release
*/
extern int dmc_succ(const pdmcState s,
                    pdmcState *succ, size_t *succ_size) {

    int tr, marking_pri;

    Result_p current_transition;
    Result_p next_transition;
    Event_p ev_p, nev_p, copy_of_ev_p;

    *succ = NULL;
    *succ_size = 0;
    /*  succ = malloc (sizeof(EventPropSucc)* (*succ_size)); */


    /* Set current marking to argument "s" */
    dmc_set_state(s, 0);

    /* calculate list of enabled transitions */
    /* Also calculates cur_priority for current marking i.e. highest priority of enabled trans. */
    enabled_head = NULL;
    my_initialize_en_list();
    current_transition = enabled_head;

    /* iterate over all enabled transitions */
    while (current_transition != NULL) {
        next_transition = current_transition->next;
        tr = GET_TRANSITION_INDEX(current_transition->list);

        /*       int i; */
        /*       AtomicProp eprop = EVENT_TRUE; */
        /*       for (i=0; i<tab_event_prop_size ; i++) { */
        /* 	if (tr == tab_event_prop[i]) { */
        /* 	  eprop = tr; */
        /* 	  break; */
        /* 	} */
        /*       } */

        /* only for transistions with current/highest priority*/
        if (tabt[tr].pri == cur_priority) {
            copy_of_ev_p = get_new_event(tr);
            ev_p = current_transition->list;

            /* for all enabled instances  */
            while (ev_p != NULL) {
                nev_p = ev_p->next;
                copy_event(copy_of_ev_p, ev_p);
                fire_trans(ev_p);

#ifdef SWN
#ifdef SYMBOLIC
                get_canonical_marking();
#endif
#endif
                (void) marking_to_string();
                tro = 0;
                //	      fprintf(stderr,"Concrete2 : %ld !",current_marking->marking->ordinary);
                //  insert_tree(&root,&h,f_mark,length,d_ptr);
                /// TODO
                pdmcState ret = dmcStateNew(cache_string, d_ptr, length);

                /* Add this new state to list of successors */
                (*succ_size)++ ;
                *succ = realloc(*succ, sizeof(dmcState) * (*succ_size));
                memcpy((*succ + (*succ_size) - 1), ret, sizeof(dmcState)) ;

                dmc_set_state(s, 1);

                /* next instance for current transition */
                ev_p = nev_p;

            }/* for all enabled instances  */

            dispose_old_event(copy_of_ev_p);
        }/* only for transistions with highest priority */

        current_transition = next_transition;
    }/* iterate over all enabled transitions */

    dmc_garbage_collect(enabled_head);

    return 0;
}

void dmc_succ_free(pdmcState *tab) {
    free(tab);
}


#endif
