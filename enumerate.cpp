#include<iostream>
#include <vector>
#include "eclat.h"
#include "timetrack.h"
#include "calcdb.h"
#include "eqclass.h"
#include "stats.h"
#include "maximal.h"
#include "chashtable.h"

using namespace std;
//extern vars
extern Dbase_Ctrl_Blk *DCB;
extern Stats stats;
MaximalTest maxtest; //for maximality & closed (with cmax) testing
cHashTable hashtest; //for closed (with chash) testing

//extern functions
extern void form_closed_f2_lists(Eqclass *eq);
extern void form_f2_lists(Eqclass *eq);
extern subset_vals get_intersect(idlist *l1, idlist *l2,
                                 idlist *join, int minsup=0);
extern subset_vals get_diff (idlist *l1, idlist *l2, idlist *join);
extern subset_vals get_join(Eqnode *l1, Eqnode *l2, Eqnode *join, int iter);
extern void get_max_join(Eqnode *l1, Eqnode *l2, Eqnode *join, int iter);

void print_tabs(int depth)
{
   for (int i=0; i < depth; ++i)
      cout << "\t";
}

static bool notfrequent (Eqnode &n){
  //cout << "IN FREQ " << n.sup << endl;
  if (n.support() >= MINSUPPORT) return false;
  else return true;
}

void enumerate_max_freq(Eqclass *eq, int iter, idlist &newmax)
{
   int nmaxpos;

   TimeTracker tt;
   Eqclass *neq;
   list<Eqnode *>::iterator ni, nj;
   Eqnode *join;


   bool extend = false;
   bool subset = false;

   eq->sort_nodes();
   nmaxpos = newmax.size(); //initial newmax pos

   //cout << "CMAX " << *eq;
   //print_tabs(iter-3);

   //cout << "NUMJOINx: " << Stats::numjoin << endl;
   //cout << "ITER " << *eq;
   //cout << "NUMJOINy: " << Stats::numjoin << endl;

   //if ni is a subset of maxset then all elements after ni must be
   for (ni = eq->nlist().begin(); ni != eq->nlist().end() && !subset; ++ni){
      tt.Start();

      neq = new Eqclass;
      neq->set_prefix(eq->prefix(),*(*ni));

      //print_tabs(iter-3);
      //cout << "prefix :";
      //neq->print_prefix();
      //cout << endl;
      //print_tabs(iter-3);
      //cout << "prevmax " << prevmax.size() << "-- " << prevmax << endl;

      subset = false;
      extend = false;
      bool res = maxtest.update_maxset(ni, eq, newmax, nmaxpos, subset);
      if (!subset || res){
            subset = maxtest.subset_test(ni, eq);
      }

      nmaxpos = newmax.size();

      //for (nj=ni; nj != eq->nlist().end(); ++nj){
      //   print_tabs(iter-3);
      //  cout << *(*nj);
      //}

      //print_tabs(iter-3);
      //cout << (subset ? "SUBSET":"NOTSUBSET") << endl;

      if (!subset){
         nj = ni; ++nj;
         for (; nj != eq->nlist().end(); ++nj){
            join = new Eqnode ((*nj)->val);

            get_join(*ni, *nj, join, iter);
            //cout << "ISECT " << *join;
            stats.incrcand(iter-1);
            if (notfrequent(*join)) delete join;
            else{
               extend = true;
               get_max_join(*ni, *nj, join, iter);
               neq->add_node(join);
               stats.incrlarge(iter-1);
            }
         }

         stats.incrtime(iter-1, tt.Stop());
         if (!neq->nlist().empty()){
            //print_tabs(iter-3);
            //cout << "new call " << endl;
            //cout << "FREQ " << *neq;
            enumerate_max_freq(neq, iter+1, newmax);
         }
      }
      //print_tabs(iter-3);
      if (!extend && (*ni)->maxsupport() == 0){
         if (output){
            //print_tabs(iter-3);
            neq->print_prefix() << " - " << (*ni)->support();
            if (output_idlist){
                cout << " : ";
                 cout << (*ni)->tidset;
             }
             cout << endl;
    }
         maxtest.add(eq->prefix(), (*ni)->val, (*ni)->support());
         newmax.push_back(maxtest.maxcount-1);
         stats.incrmax((iter-2));
      }
      delete neq;
   }
}

void enumerate_max_closed_freq(Eqclass *eq, int iter, idlist &newmax)
{
   int nmaxpos;

   TimeTracker tt;
   Eqclass *neq;
   list<Eqnode *>::iterator ni, nj;
   Eqnode *join;
   subset_vals sval;

   bool extend = false;
   bool subsetflg = false;

   eq->sort_nodes();

   //cout << "CMAX " << *eq;

   nmaxpos = newmax.size(); //initial newmax pos

   //if ni is a subset of maxset then all elements after ni must be
   for (ni = eq->nlist().begin(); ni != eq->nlist().end() && !subsetflg; ++ni){
      tt.Start();

      neq = new Eqclass;
      neq->set_prefix(eq->prefix(),*(*ni));

      subsetflg = false;
      extend = false;
      bool res = maxtest.update_maxset(ni, eq, newmax, nmaxpos, subsetflg);
      if (!subsetflg || res){
         subsetflg = maxtest.subset_test(ni, eq);
      }

      nmaxpos = newmax.size();

      if (!subsetflg){
         nj = ni; ++nj;
         for (; nj != eq->nlist().end();){
            join = new Eqnode ((*nj)->val);

            sval = get_join(*ni, *nj, join, iter);
            //cout << "SVAL " << (int)sval;
            //eq->print_node(*(*nj));
            //cout << endl;
            //cout << "ISECT " << *join;
            stats.incrcand(iter-1);
            if (notfrequent(*join)){
               delete join;
               ++nj;
            }
            else{
               extend = true;
               //get_max_join(*ni, *nj, join, iter);
               //neq->add_node(join);
               stats.incrlarge(iter-1);

               switch(sval){
               case subset:
                  //add nj to all elements in eq by adding nj to prefix
                  neq->prefix().push_back((*nj)->val);
                  //neq->closedsupport() = join->support();
                  delete join;
                  ++nj;
                  break;
               case notequal:
                  get_max_join(*ni, *nj, join, iter);
                  neq->add_node(join);
                  ++nj;
                  break;
               case equals:
                  //add nj to all elements in eq by adding nj to prefix
                  neq->prefix().push_back((*nj)->val);
                  //neq->closedsupport() = join->support();
                  delete *nj;
                  nj = eq->nlist().erase(nj); //remove nj
                  delete join;
                  break;
               case superset:
                  get_max_join(*ni, *nj, join, iter);
                  delete *nj;
                  nj = eq->nlist().erase(nj); //remove nj
                  //++nj;
                  neq->add_node(join);
                  break;
               }
            }
         }

         stats.incrtime(iter-1, tt.Stop());
         if (neq->nlist().size() > 1){
            enumerate_max_closed_freq(neq, iter+1, newmax);
         }
         else if (neq->nlist().size() == 1){
            nj = neq->nlist().begin();
            if ((*nj)->maxsupport() == 0){
               if (output){
                  neq->print_node(*(*nj)) << endl;
                //if (output_idlist){
                //cout << " : ";
                // cout << (*nj)->tidset;
               //}
             //cout << endl;
            }
               maxtest.add(neq->prefix(), (*nj)->val);
               newmax.push_back(maxtest.maxcount-1);
               stats.incrmax(neq->prefix().size());
            }
         }
         else if (extend && (*ni)->maxsupport() == 0){
            if (output)
               neq->print_prefix() << endl;
            maxtest.add(neq->prefix(), -1);
            newmax.push_back(maxtest.maxcount-1);
            stats.incrmax(neq->prefix().size()-1);
         }
      }

      if (!extend && (*ni)->maxsupport() == 0){
         if (output)
            neq->print_prefix() << endl;
         maxtest.add(eq->prefix(), (*ni)->val);
         newmax.push_back(maxtest.maxcount-1);
         stats.incrmax(neq->prefix().size()-1);
      }

      delete neq;
   }
}

void enumerate_closed_freq(Eqclass *eq, int iter, idlist &newmax)
{
   TimeTracker tt;
   Eqclass *neq;
   int nmaxpos;
   bool cflg;
   list<Eqnode *>::iterator ni, nj;
   Eqnode *join;
   subset_vals sval;

   nmaxpos = newmax.size(); //initial newmax pos
   eq->sort_nodes();
   //print_tabs(iter-3);
   //cout << "F" << iter << " " << *eq;
   for (ni = eq->nlist().begin(); ni != eq->nlist().end(); ++ni){
      neq = new Eqclass;
      neq->set_prefix(eq->prefix(),*(*ni));

      //cout << "prefix " << neq->print_prefix() << endl;
      tt.Start();

      if (closed_type == cmax)
         maxtest.update_maxset(ni, eq, newmax, nmaxpos, cflg);

      nmaxpos = newmax.size(); //initial newmax pos
      nj = ni;
      for (++nj; nj != eq->nlist().end(); ){
         join = new Eqnode ((*nj)->val);
         sval = get_join(*ni, *nj, join, iter);
         stats.incrcand(iter-1);
         if (notfrequent(*join)){
            delete join;
            ++nj;
         }
         else{
            stats.incrlarge(iter-1);
            switch(sval){
            case subset:
               //add nj to all elements in eq by adding nj to prefix
               neq->prefix().push_back((*nj)->val);
               //neq->closedsupport() = join->support();
               //cout << "SUSET " << *join << endl;
               delete join;
               ++nj;
               break;
            case notequal:
               if (closed_type == cmax) get_max_join(*ni, *nj, join, iter);
               neq->add_node(join);
               ++nj;
               break;
            case equals:
               //add nj to all elements in eq by adding nj to prefix
               neq->prefix().push_back((*nj)->val);
               //neq->closedsupport() = join->support();
               delete *nj;
               nj = eq->nlist().erase(nj); //remove nj
               //cout << "EQUAL " << *join << endl;
               delete join;
               break;
            case superset:
               if (closed_type == cmax) get_max_join(*ni, *nj, join, iter);
               delete *nj;
               nj = eq->nlist().erase(nj); //remove nj
               //++nj;
               neq->add_node(join);
               break;
            }
         }
      }

      cflg = true;
//       if (output){
//          cout << "yy ";
//          neq->print_prefix();
//          cout << endl;
//       }

      if (closed_type == cmax){
         cflg = maxtest.check_closed(*ni);
         if (cflg){
            maxtest.add(neq->prefix(), -1, (*ni)->support());
            newmax.push_back(maxtest.maxcount-1);
         }
      }
      else if (closed_type == chash){
         cflg = hashtest.add(neq->prefix(), -1, (*ni)->support(),
                             (*ni)->hval);
      }

      if (cflg){
         stats.incrmax(neq->prefix().size()-1);
         if (output){
            neq->print_prefix(true);
            cout << "- " << (*ni)->sup;
            if (output_idlist){
                cout << " : ";
                 cout << (*ni)->tidset;
             }
             cout << endl;
 }
      }

      stats.incrtime(iter-1, tt.Stop());
//       if (output) cout << "xx " << neq->nlist().size()
//                        << "-- " << *neq << endl;
      if (neq->nlist().size() > 1){
         enumerate_closed_freq(neq, iter+1, newmax);
      }
      else if (neq->nlist().size() == 1){
         cflg = true;
         if (closed_type == cmax){
            cflg = maxtest.check_closed(neq->nlist().front());
            if (cflg){
               maxtest.add(neq->prefix(), neq->nlist().front()->val,
                           neq->nlist().front()->sup);
               newmax.push_back(maxtest.maxcount-1);
            }
         }
         else if (closed_type == chash){
            cflg = hashtest.add(neq->prefix(), neq->nlist().front()->val,
                                neq->nlist().front()->sup,
                                neq->nlist().front()->hval);
         }

         if (cflg){
            if (output) cout << *neq;
            stats.incrmax(neq->prefix().size());
         }
      }
      delete neq;
   }
}

void enumerate_freq(Eqclass *eq, int iter)
{
   TimeTracker tt;
   Eqclass *neq;
   list<Eqnode *>::iterator ni, nj;
   Eqnode *join;

   for (ni = eq->nlist().begin(); ni != eq->nlist().end(); ++ni){
      neq = new Eqclass;
      neq->set_prefix(eq->prefix(),*(*ni));
      tt.Start();
      nj = ni;
      for (++nj; nj != eq->nlist().end(); ++nj){
         join = new Eqnode ((*nj)->val);
         get_join(*ni, *nj, join, iter);
         stats.incrcand(iter-1);
         if (notfrequent(*join)) delete join;
         else{
            neq->add_node(join);
            stats.incrlarge(iter-1);
         }
      }
      stats.incrtime(iter-1, tt.Stop());
      if (output) cout << *neq;
      if (neq->nlist().size()> 1){
         enumerate_freq(neq, iter+1);
      }
      delete neq;
   }
}

void form_closed_f2_lists(Eqclass *eq)
{
   static vector<bool> *bvec = NULL;
   if (bvec == NULL){
      bvec = new vector<bool>(DCB->NumF1, true);
   }

   subset_vals sval;
   list<Eqnode *>::iterator ni;
   Eqnode *l1, *l2;
   int pit, nit;
   TimeTracker tt;
   bool extend = false;
   bool cflg;

   tt.Start();
   pit = eq->prefix()[0];
   l1 = DCB->ParentClass[pit];

   if (!(*bvec)[pit]){
      eq->nlist().clear();
     return;
   }

   if (alg_type == maxcharm){
      cflg = maxtest.subset_test_f2(eq);
      if (cflg){
         eq->nlist().clear();
         return;
      }
   }

//    cout << "PARENT " << DCB->FreqIdx[pit] << " : ";
//    for (ni=eq->nlist().begin(); ni != eq->nlist().end(); ++ni){
//       nit = (*ni)->val;
//       cout << " " << DCB->FreqIdx[nit];
//    }
//    cout << endl;


   for (ni=eq->nlist().begin(); ni != eq->nlist().end(); ){
      nit = (*ni)->val;
      if (!(*bvec)[nit]){
         //++ni;
         delete *ni;
         ni = eq->nlist().erase(ni); //remove ni
         continue;
      }
      l2 = DCB->ParentClass[nit];
      sval = get_join(l1, l2, (*ni), 2);
      //cout << "SVAL " << (int)sval << endl;
//       cout << "F2 " << pit << " " << nit << " -- "
//            <<DCB->FreqIdx[pit] << " " << DCB->FreqIdx[nit]
//            << " -- " << l1->sup << " " << l2->sup << " " << (*ni)->sup
//            << " -- " << l1->val << " " << l2->val << endl;
      switch(sval){
      case subset:
         //add nj to all elements in eq by adding nj to prefix
         eq->prefix().push_back((*ni)->val);
         extend = true;
         //eq->closedsupport() = (*ni)->support();
         delete *ni;
         ni = eq->nlist().erase(ni); //remove ni
         //cout << "CAME HERE " << eq->nlist().size() << endl;
         break;
      case notequal:
         if (alg_type == maxcharm || closed_type == cmax)
            get_max_join(l1, l2, (*ni), 2);
         ++ni;
         break;
      case equals:
         //add nj to all elements in eq by adding nj to prefix
         eq->prefix().push_back((*ni)->val);
         extend = true;
         //eq->closedsupport() = (*ni)->support();
         delete *ni;
         ni = eq->nlist().erase(ni); //remove ni
         (*bvec)[nit] = false;
         //cout << "CAME HERE " << eq->nlist().size() << endl;
         break;
      case superset:
         (*bvec)[nit] = false;
         if (alg_type == maxcharm || closed_type == cmax)
            get_max_join(l1, l2, (*ni), 2);
         ++ni;
         break;
      }
   }

   if (alg_type == charm){

      cflg = true;
      if (closed_type == cmax){
         cflg = maxtest.check_closed(l1);
         if (cflg) maxtest.add(eq->prefix(), -1, l1->support());
      }
      else if (closed_type == chash){
         cflg = hashtest.add(eq->prefix(), -1, l1->support(),
                             l1->hval);
      }

      if (cflg){
         stats.incrmax(eq->prefix().size()-1);
         if (output){
//             cout << "ff ";
            eq->print_prefix(true);
            cout << "- " << l1->sup;
             if (output_idlist){
                cout << " : ";
                 cout << l1->tidset;
             }
             cout << endl;
         }
      }


      if (eq->nlist().size() == 1){
         cflg = true;
         if (closed_type == cmax){
            cflg = maxtest.check_closed(eq->nlist().front());
            if (cflg){
               maxtest.add(eq->prefix(), eq->nlist().front()->val,
                           eq->nlist().front()->sup);
            }
         }
         else if (closed_type == chash){
            cflg = hashtest.add(eq->prefix(), eq->nlist().front()->val,
                                eq->nlist().front()->sup,
                                eq->nlist().front()->hval);
         }

         if (cflg){
            if (output){
               cout << *eq;
            }

            stats.incrmax(eq->prefix().size());
         }
         eq->nlist().clear();
      }
   }
   else if (alg_type == maxcharm){
      if (eq->nlist().empty() && l1->maxsupport() == 0){
         if (output){
            eq->print_prefix();
             if (output_idlist){
                cout << " : ";
                 cout << l1->tidset;
             }
             cout << endl;

         }
         maxtest.add(eq->prefix(), -1);
         stats.incrmax(eq->prefix().size()-1);
      }
      else if (eq->nlist().size() == 1){
         l1 = eq->nlist().front();
         if (l1->maxsupport() == 0){
            if (output)
               eq->print_node(*l1) << endl;
            maxtest.add(eq->prefix(), l1->val);
            stats.incrmax(eq->prefix().size());
         }
         eq->nlist().clear();
      }
   }

   //cout << "F2 " << *eq << endl;
   stats.incrtime(1,tt.Stop());
   //delete l1;
}



void form_f2_lists(Eqclass *eq)
{
   list<Eqnode *>::iterator ni;
   Eqnode *l1, *l2;
   int pit, nit;
   TimeTracker tt;

   tt.Start();
   //cout << "F2 " << *eq << endl;
   pit = eq->prefix()[0];
   l1 = DCB->ParentClass[pit];
   for (ni=eq->nlist().begin(); ni != eq->nlist().end(); ++ni){
      nit = (*ni)->val;
      l2 = DCB->ParentClass[nit];

      get_join(l1, l2, (*ni), 2);
      if (alg_type == basicmax) get_max_join(l1, l2, (*ni), 2);
//       cout << "LISTS " << pit << " " << nit << " "
//            <<DCB->FreqIdx[pit] << " " << DCB->FreqIdx[nit]
//            << " " << l1->sup << " " << l2->sup << " " << (*ni)->sup <<endl;
      //cout << "f2prefix " << eq->prefix() << endl;
      //cout << "f2 " << *ins;
      //cout << "F2 " << l1->maxset << " xx  " << l2->maxset << endl;
   }
   //delete l1;
   stats.incrtime(1,tt.Stop());
}


