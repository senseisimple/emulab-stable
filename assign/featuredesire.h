/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2004-2009 University of Utah and the Flux Group.
 * All rights reserved.
 */

#ifndef __FEATUREDESIRE_H
#define __FEATUREDESIRE_H

#include "common.h"

#include "port.h"
#include "fstring.h"

#include <iostream>
using namespace std;


namespace featuredesire {
    // The weight at which a feature or desire triggers a violations if
    // unstatisfied or unused
    const float FD_VIOLATION_WEIGHT = 1.0;
    
    enum fd_type {
	FD_TYPE_NORMAL,
	FD_TYPE_LOCAL_ADDITIVE,
	FD_TYPE_GLOBAL_ONE_IS_OKAY,
	FD_TYPE_GLOBAL_MORE_THAN_ONE
    };
};

/*
 * This small class is used to describe policies in force regarding features
 * and desires. The idea behind putting this it its own class is that you want
 * to be able to apply baiscally the same policies to features and desires, but
 * treat them seperateley.
 */
class tb_featuredesire_policy {
public:
    tb_featuredesire_policy(): allowable(true), limited_use(false),
    max_use(0.0f), min_use(0.0f)  { ; };
    /*
     * Functions for maintaining FD policy state. Inline, since they're
     * so simple.
     */
    void allow()                   { allowable = true; };
    void disallow()                { allowable = false; }
    bool is_allowable()            { return this->allowable; };
    void limit_use(double howmuch) { limited_use = true; max_use = howmuch; };
    void unlimit_use()             { limited_use = false; max_use = 0.0f; }
    bool is_limited()              { return limited_use; }
    double get_limit()             { return max_use; }
    
    friend ostream &operator<<(ostream &o, const tb_featuredesire_policy &fdp);
    
private:
    // Indicates whether or not we are allowed to use this feature or
    // desire, and if so, how much of it we are allowed to use. Note that
    // while these limits are global, this is different from using a global
    // FD - it is a policy knob that can be applied in the ptop file
    bool allowable;
    bool limited_use;
    double max_use;
    // Note: min_use not implemented, as it's not clear what the point
    // would be
    double min_use;
};

/*
 * Base class for features and desires.
 */
class tb_featuredesire {
    public:

	/*
	 * Note: Constructor is below - use get_featuredesire_obj instead
	 */

	~tb_featuredesire() { ; }

	/*
	 * Get the object for a particular feature/desire - returns NULL if
	 * the feature/desire does not exist.
	 */
	static tb_featuredesire *get_featuredesire_by_name(const fstring name);

	/*
	 * Get the object for a particular feature/desire if it already exists,
	 * or creates a new one if it does not. DEPRECATED, use the next version
	 * below.
	 */
	static tb_featuredesire *get_or_create_featuredesire(const fstring name);
        
       /*
	* Get the object for a particular feature/desire if it already exists,
	* or creates a new one if it does not.
        */
        static tb_featuredesire *get_or_create_featuredesire(const fstring name,
					    featuredesire::fd_type type);
	/*
	 * Silly accessor functions
	 */
	inline bool  is_global()        const { return global;          }
	inline bool  is_local()         const { return local;           }
	inline bool  is_l_additive()	const { return l_additive;	}
	inline bool  is_g_one()		const { return g_one_is_okay;	}
	inline bool  is_g_more()	const { return g_more_than_one; }
	inline int   global_use_count() const { return in_use_globally; }
	inline fstring name()           const { return my_name;         }

	/*
	 * Operators, primarily for use with the STL
	 */
	inline bool operator==(const tb_featuredesire &o) const {
	    return (id == o.id);
	}

	inline bool operator<(const tb_featuredesire &o) const {
	    return (id < o.id);
	}

	friend ostream &operator<<(ostream &o, const tb_featuredesire &fd);

	/*
	 * Functions for maintaining state for global FDs
	 */
	void add_global_user(int howmany = 1);
	void remove_global_user(int howmany = 1);
    
	/*
	 * Simple bookkeeping
	 */
	void add_desire_user(double weight);
	int get_desire_users();
	double get_desire_total_weight();
    
	/*
	 * Functions for manipulating the policies for features or desires with
	 * this name.
	 * NOTE: Only desire policies implement for now
	 */
	void disallow_desire();
	void allow_desire();
	void limit_desire(double limit);
	void unlimit_desire();
    
	/*
	 * Check to see if any desires violate policies - this is checkable at
	 * any time (after all desires and policies have been set up), because
	 * it is not dependant on the current mapping. Returns true if
	 * everything is okay, false if not.
	 * Feature policy violations, which are not yet implemented, will be
	 * much harder because of thise.
	 */
	static bool check_desire_policies();
    
    private:
	/*
	 * This is private, so that we can force callers to go through the
	 * static get_featuredesire_obj function which uses the existing desire
	 * if there is one
	 */
	explicit tb_featuredesire(fstring _my_name);
    
        /*
	 * This version of the constructor takes the type of the FD explicitly,
	 * no mucking around with trying to find "flags" in the name.
	 */
	explicit tb_featuredesire(fstring _my_name,
				  featuredesire::fd_type _fd_type);

	// Globally unique identifier
	int id;

	// String name of this FD, used for debugging purposes only
	fstring my_name;

	// Flags
	bool global;          // Whether this FD has global scope
	bool g_one_is_okay;   // If global, we can have one without penalty
	bool g_more_than_one; // If global, more than one doesn't incur
			      // additional penalty
	bool local;           // Whether this FD has local scope
	bool l_additive;      // If a local FD, is additive

	// Counts how many instances of this feature are in use across all
	// nodes - for use with global features
	int in_use_globally;

	typedef map<fstring,tb_featuredesire*> name_featuredesire_map;
	static name_featuredesire_map featuredesires_by_name;
    
	/*
	 * Policies for using features and desires with the name. NOTE: Only
	 * feature policies implemented yet, as they are easier to check for.
	 */
	tb_featuredesire_policy feature_policy;
	tb_featuredesire_policy desire_policy;
    
	/*
	 * Some simple bookkeeping stuff, currently only used to enforce
	 * policies.
	 */
	int desire_users;
	double desire_total_weight;
    
        /*
	 * Used to assign unque IDs to each feature/desire
	 */
        static int highest_id;
};

/*
 * This class stores information about a particular feature or desire for a
 * particular node, rather than global information about the feature or desire.
 */
class tb_node_featuredesire {
    public:
        /*
	 * This version of the constructor takes "legacy" names, with the
	 * special flags at the beginning, and parses them
	 */
	tb_node_featuredesire(fstring _name, double _weight);

	/*
	 * This version of the constructor passes all type information
	 * explicitly, and leaves the name alone
	 */
	tb_node_featuredesire(fstring _name, double _weight,
			      bool _violatable, featuredesire::fd_type _type);
    
	~tb_node_featuredesire() { ; }

	/*
	 * Operators, mostly for use with the STL
	 */
	const bool operator==(const tb_node_featuredesire &o) const {
	    // Note: Compares that two FDs are have the same name/ID, but NOT
	    // that they have the same weight - see equivalent() below for that
	    return(*featuredesire_obj == *(o.featuredesire_obj));
	}

	const bool operator<(const tb_node_featuredesire &o) const {
	    return(*featuredesire_obj < *(o.featuredesire_obj));
	}

	// Since we have to use == to compare names, for the STL's sake, this
	// function checks to make sure that the node-specific parts are
	// equivalent too
	const bool equivalent(const tb_node_featuredesire &o) const {
	    return ((*this == o) && (weight == o.weight) &&
		    (violateable == o.violateable));
	}

	/*
	 * Silly accesors
	 */
	inline const bool   is_violateable() const { return violateable; }
	inline const double cost()           const { return weight;      }
	inline const double used()	     const { return used_local_capacity; }

	/*
	 * Proxy functions for the stuff in tb_featuredesire
	 */
	const fstring name()      const { return featuredesire_obj->name();      }
	const bool  is_local()  const { return featuredesire_obj->is_local();  }
	const bool  is_global() const { return featuredesire_obj->is_global(); }
	const bool  is_l_additive() const {
	    return featuredesire_obj->is_l_additive();
	}
	void add_desire_user(double weight) const { 
	    featuredesire_obj->add_desire_user(weight); 
	}

	score_and_violations add_global_user() const;
	score_and_violations remove_global_user() const;

	/*
	 * Functions for tracking local features/desires
	 */
	score_and_violations add_local(double amount);
	score_and_violations subtract_local(double amount);

    protected:

	double weight;
	bool violateable;
	tb_featuredesire *featuredesire_obj;
	double used_local_capacity;
};

/*
 * Types to hold virtual nodes' sets of desires and physical nodes' sets of
 * features
 */
typedef slist<tb_node_featuredesire> node_feature_set;
typedef slist<tb_node_featuredesire> node_desire_set;
typedef slist<tb_node_featuredesire> node_fd_set;

/*
 * Kind of like an iterator, but not quite - used for going through a virtual
 * node's desires and a physical node's features, and deterimining which are
 * only in one set, and which are in both
 */
class tb_featuredesire_set_iterator {
    public:
	/*
	 * Constructors
	 */
	tb_featuredesire_set_iterator(node_fd_set::iterator _begin1,
		node_fd_set::iterator _end1,
		node_fd_set::iterator _begin2,
		node_fd_set::iterator _end2);

	// Enum for indicating which set(s) an element belongs to
	typedef enum { FIRST_ONLY, SECOND_ONLY, BOTH } set_membership;

	// Return whether or not we've iterated to the end of both sets
	bool done() const;

	// If we have a membership() of BOTH, do they pass the equivalence
	// test?
	bool both_equiv() const;

	// Is either of the two elements violateable?
	bool either_violateable() const;

	// Iterate to the next element of the set
	void operator++(int);

	// Return the member of the set we're currently iterating to
	const tb_node_featuredesire &operator*() const {
	    return *current;
	}
	
	// Return the member of the set we're currently iterating to
	const tb_node_featuredesire *operator->() const {
	    return &*current;
	}

	// Return the set membership of the current element
	const set_membership membership() const {
	    return current_membership;
	}

	// Give out the iterators to the two elements, so that they can be
	// operated upon
	node_fd_set::iterator &first_iterator()  { return it1; }
	node_fd_set::iterator &second_iterator() { return it2; }
	
    private:
	node_fd_set::iterator it1, end1;
	node_fd_set::iterator it2, end2;
	node_fd_set::iterator current;
	set_membership current_membership;
};

#endif
