(*
 * ordering-to-tree.ml - Turn an ordering into a tree
 *)

(*
 * Functions that should be exported by any scoring module. I don't know how to
 * express this in the ML type system, but the 'a and 'b below should be the
 * same across all functions. The 'blob' is a black-box type that I won't look
 * into the internals of, representing an RES set, an ORTC-type structure, or
 * whatever.
 *)
(* Make the blob for a set of nodes *)
type ('a,'b) blob_from_set_f = ('b array array -> Dre.nodeset -> 'a);;
(* How good is the resulting blob? Greater ints are better *)
type ('a) score_blob_f = ('a -> int);;
(* Combine the blobs for two sets of nodes *)
type ('a,'b) combine_blobs_f = ('b array array
    -> Dre.nodeset -> Dre.nodeset -> 'a -> 'a -> 'a);;

let debug (str : string) : unit =
    ()
;;

(* Bindings for RES scoring - make changeable *)
let (blob_from_set : ('a,'b) blob_from_set_f) = Dre.setwise_res;;
let (score_blob : 'a score_blob_f) = Dre.score_res;;
let (combine_blobs : ('a,'b) combine_blobs_f) = Dre.merge_res;;

(* TODO - Better command-line parsing *)
(* let graph = Graph.read_graph_file Sys.argv.(1);; *)
let (graph, headers) = Graph.read_subgraph_file Sys.argv.(1);;
debug ("Graph size " ^ (string_of_int (List.length graph.Graph.nodes)));;
debug ("Edges " ^ (string_of_int (List.length graph.Graph.edges)));;
let naming = Naming.read_naming_file Sys.argv.(2);;

let ordering = Naming.ordering_of_naming naming;;
(* let hops = Dijkstra.get_all_first_hops graph;; *)
let hops = if Array.length Sys.argv > 3 then
    Dijkstra.read_fh_file Sys.argv.(3) (List.length graph.Graph.nodes)
else
    Dijkstra.get_all_first_hops graph;;

let do_nothing () : unit =
    raise(Failure "I'm not supposed to be called!");
    ()
;;

(* XXX! *)
let dummy_blob = Dre.ISet.empty;;

(* Tree we're constructing *)
type tree_node = NoNode | TreeNode of (int * tree_node * tree_node);;

let rec tree_depth (tree : tree_node) : int =
    match tree with
      NoNode -> 0
    | TreeNode(_,l,r) -> 1 + (max (tree_depth l) (tree_depth r))
;;

(* (left neighbor, right neighbor) *)
(* type combination = (int * int);; *)
type 'a heap_entry = { left : int;
                    right : int;
                    h_blob : 'a; (* XXX *)
                    mutable remove : (unit -> unit) };;
(*type combination = { left : int;
                     right : int;
                     mutable remove_left : (unit -> unit);
                     mutable remove_right : (unit -> unit) };; *)
let dummy_heap_entry = { left = -1; right = 1; remove = do_nothing; h_blob = dummy_blob };;


(* (id, nodes, tree entry) *)
(* type ordering_entry = (int * Dre.nodeset * tree_node);; *)
type 'a ordering_entry = { id : int;
                           nodes : Dre.nodeset;
                           tree : tree_node;
                           o_blob : 'a; (* XXX *)
                           mutable entry : 'a heap_entry };;
type 'a ordering = 'a ordering_entry Linklist.t;;

(* Make an ordering list, in the form we need, from the ordering file *)
(*
let olist = List.map
                (fun x -> let node_set = Dre.ISet.singleton x in
                          { id = x;
                            nodes = node_set;
                            tree = TreeNode(x,NoNode,NoNode);
                            o_blob = blob_from_set  hops node_set;
                            entry = dummy_heap_entry })
                (Array.to_list ordering);;
*)
let olist = List.fold_left
                (fun (next : 'a ordering_entry Linklist.t)
                     (this : int) -> let node_set = Dre.ISet.singleton this in
                            let o_elt = { id = this;
                                nodes = node_set;
                                tree = TreeNode(this,NoNode,NoNode);
                                o_blob = blob_from_set  hops node_set;
                                entry = dummy_heap_entry } in
                            let l_elt = Linklist.Node({ Linklist.next = next;
                                            Linklist.contents = o_elt;
                                            Linklist.prev = Linklist.None }) in
                            (match next with
                              Linklist.None -> ()
                            | Linklist.Node(x) -> x.Linklist.prev <- l_elt);
                            l_elt)
                Linklist.None (Array.to_list ordering);;

(* Linklist.iter olist (fun x -> print_endline (string_of_int x.id));; *)

(* Take an ordering and create a heap (priority queue) of all legal
 * combinations *)
let initialize_heap (sets : 'a ordering) : 'a heap_entry Heap.heap =
    (* Make an empty heap which we'll mutate in the helper *)
    (* print_endline "Making a heap"; *)
    let heap = Heap.make_heap dummy_heap_entry in
    let rec combine_with_neighbors (e : 'a ordering_entry) (l : 'a ordering) : unit =
        match l with
          Linklist.None -> ()
        | Linklist.Node(elt) -> begin
            (*
            let (id1,s1,_) = e in
            let (id2,s2,_) = x in
            *)
            let x = elt.Linklist.contents in
            let id1 = e.id in
            let id2 = x.id in
            let s1 = e.nodes in
            let s2 = x.nodes in
            let s3 = Dre.ISet.union s1 s2 in
            let blob = blob_from_set hops s3 in
            (* XXX - Need a heap that gets us the max, not the min! *)
            debug ("Remove function for " ^ (string_of_int id1) ^ ", "
            ^ (string_of_int id2) ^ " put into " ^ (string_of_int x.id));
            let heap_entry = { left = id1; right = id2; remove = do_nothing; h_blob = blob } in
            let remove = Heap.insert_remove heap (0 - (score_blob blob)) heap_entry in
            x.entry <- heap_entry;
            heap_entry.remove <- remove;
            combine_with_neighbors x elt.Linklist.next 
        end
    in
    (match sets with
      Linklist.None -> raise (Failure "Empty list in initialize_heap")
    | Linklist.Node(elt) -> combine_with_neighbors elt.Linklist.contents elt.Linklist.next);
    heap
;;

let next_id = ref ((Array.length hops) + 1);;

let heap = initialize_heap olist;;
(* Combine two neighbors by creating a parent, and replacing them with it *)
let rec combine_neighbors (ordering : 'a ordering) (first : int) (second : int) :
        'a ordering =
    (* TODO - Could almost certainly be made faster *)
    match ordering with
      Linklist.None -> raise (Failure ("Empty list to combine_neighbors for " ^
                             (string_of_int first) ^ " and " ^
                             (string_of_int second)))
    | Linklist.Node(elt) when elt.Linklist.contents.id = first -> begin
            debug ("Combining " ^ (string_of_int first) ^ " and " ^
                (string_of_int second));
            (* Get the first and second siblings, and their links to the rest
             * of the ordering *)
            let first_sibling = elt.Linklist.contents in
            let (second_sibling, rest) = match elt.Linklist.next with
                          Linklist.None -> raise (Failure "Bad pair")
                        | Linklist.Node(elt2) ->
                                (elt2.Linklist.contents, elt2.Linklist.next) in
            let prior = elt.Linklist.prev in

            (* Make a new parent *)
            let id = !next_id in
            next_id := !next_id + 1;
            let (newtree : tree_node) =
                TreeNode(id,first_sibling.tree,second_sibling.tree) in
            (*
            debug ((string_of_int (tree_depth first_sibling.tree)) ^ " + "
                ^ (string_of_int (tree_depth second_sibling.tree)) ^ " = " ^
                (string_of_int (tree_depth newtree)));
            *)
            let combined_blob =
                combine_blobs hops first_sibling.nodes second_sibling.nodes
                              first_sibling.o_blob second_sibling.o_blob in
            let parent = {id = id;
                          nodes = Dre.ISet.union first_sibling.nodes
                                                 second_sibling.nodes;
                          tree = newtree;
                          entry = dummy_heap_entry;
                          o_blob = combined_blob } in
            debug ("Adding " ^ (string_of_int id));

            (*
            debug "Heap: ";
            Heap.iterw heap (fun x y -> debug ((string_of_int x) ^ " = "
                ^ (string_of_int y.left) ^ ", " ^ (string_of_int y.right)));
            *)

            (* Remove the three affected items from the heap *)
            debug ("Removing " ^ (string_of_int
                second_sibling.entry.left) ^ ", " ^ (string_of_int
                second_sibling.entry.right) ^ " (" ^ (string_of_int
                second_sibling.id ^ ")"));
            second_sibling.entry.remove ();
            (match rest with 
               Linklist.None -> ()
            | Linklist.Node(elt) ->
                debug ("Removing " ^ (string_of_int
                    elt.Linklist.contents.entry.left) ^ ", " ^ (string_of_int
                    elt.Linklist.contents.entry.right) ^ " (" ^ (string_of_int
                    elt.Linklist.contents.id ^ ")"));
                elt.Linklist.contents.entry.remove());

            (match prior with
               Linklist.None -> ()
            | Linklist.Node(_) ->
                debug ("Removing " ^ (string_of_int
                    first_sibling.entry.left) ^ ", " ^ (string_of_int
                    first_sibling.entry.right) ^ " (" ^ (string_of_int
                    first_sibling.id ^ ")"));
                first_sibling.entry.remove ());

            debug "Heap: ";
            Heap.iter heap (fun x -> debug ((string_of_int x.left) ^
                ", " ^ (string_of_int x.right)));

            let p_entry = Linklist.Node({ Linklist.prev = elt.Linklist.prev;
                            Linklist.contents = parent;
                            Linklist.next = rest }) in

            (* Try combinining this node with its new neighbors *)
            (match rest with 
               Linklist.None -> ()
            | Linklist.Node(elt) -> begin
                let node = elt.Linklist.contents in
                let combined_blob = combine_blobs hops node.nodes parent.nodes
                                    node.o_blob parent.o_blob in
                let blob_score = 0 - (score_blob combined_blob) in
                let heap_entry = { left = parent.id;
                                   right = node.id;
                                   remove = do_nothing;
                                   h_blob = combined_blob } in
                debug ("Adding heap entry for " ^ (string_of_int heap_entry.left)
                    ^ ", " ^ (string_of_int heap_entry.right));
                let remove = Heap.insert_remove heap blob_score heap_entry in
                heap_entry.remove <- remove;
                node.entry <- heap_entry;
                elt.Linklist.prev <- p_entry 
            end);
            (match prior with 
               Linklist.None -> ()
            | Linklist.Node(elt) -> begin
                let node = elt.Linklist.contents in
                let combined_blob = combine_blobs hops node.nodes parent.nodes
                                    node.o_blob parent.o_blob in
                let blob_score = 0 - (score_blob combined_blob) in
                let heap_entry = { left = node.id;
                                   right = parent.id;
                                   remove = do_nothing;
                                   h_blob = combined_blob } in
                debug ("Adding heap entry for " ^ (string_of_int heap_entry.left)
                    ^ ", " ^ (string_of_int heap_entry.right));
                let remove = Heap.insert_remove heap blob_score heap_entry in
                heap_entry.remove <- remove;
                parent.entry <- heap_entry
            end);

            (* Return the ordering element *)
            p_entry
            
        end
    | Linklist.Node(elt) -> Linklist.Node({ Linklist.prev = elt.Linklist.prev;
                              Linklist.contents = elt.Linklist.contents;
                              Linklist.next = combine_neighbors elt.Linklist.next first second })
;;

let estimated_routes = ref ((Array.length hops) * (Array.length hops));;
(* Do n-1 cobminations, until there is only one element left *)
let rec greedy_combine (ordering : 'a ordering) : tree_node =
    debug "greedy_combine called";
    Linklist.iter ordering (fun x -> debug (string_of_int x.id));
    match ordering with
      Linklist.None -> raise (Failure "Empty ordering given to greedy_combine") 
    | Linklist.Node(elt) when elt.Linklist.next = Linklist.None ->
            elt.Linklist.contents.tree (* We're done! *)
    | _ -> begin
        (* TODO - Speed up by NOT calling initialize_heap every time *)
        (* let heap = initialize_heap ordering in *)
        let (score,entry) = Heap.min heap in
        let neworder = combine_neighbors ordering entry.left entry.right in
        estimated_routes := !estimated_routes + score;
        greedy_combine neworder
    end
in

(* Given a tree, return an array that shows every node's placment in it *)
let tree_placement (howmany : int) (tree : tree_node) : (int * int32 array) =
    let locations = Array.make howmany Int32.minus_one in
    let rec helper (tree : tree_node) (depth : int) (sofar : int32) : int =
        (*
        if depth >= 32 then
            raise (Failure "Tree too deep"); *)
        match tree with
          NoNode -> -1
        | TreeNode(id,left,right) ->
                if left == NoNode && right == NoNode then begin
                    (* Leaf node *)
                    locations.(id) <- sofar;
                    depth
                end else
                    (* Get the children's IDs *)
                    let levelval = Int32.shift_left Int32.one (31 - depth) in
                    let left_val = sofar in
                    let right_val = Int32.logor sofar levelval in
                    let nextdepth = depth + 1 in

                    (* Recurse! *)
                    max (helper left nextdepth left_val)
                        (helper right nextdepth right_val)
    in
    let depth = (helper tree 0 Int32.zero) in
    (depth, locations)
in

let root = greedy_combine olist in
let rec print_tree (root : tree_node) =
    match root with
       NoNode -> ()
    | TreeNode(id,left,right) -> begin
            print_tree left;
            output_string stdout ((string_of_int id) ^ "\n");
            print_tree right
        end
in
let (depth,placement) = tree_placement (Array.length hops) root in
print_endline ("bits " ^ (string_of_int depth));
print_endline ("routes " ^ (string_of_int !estimated_routes));
Array.iter (fun x -> Printf.printf "%0lu\n" (Int32.shift_right_logical x (32 - depth))) placement
