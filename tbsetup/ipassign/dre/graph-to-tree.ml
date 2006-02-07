(*
 * Command-line args
 *)
let use_ortc = ref false;;
let graphfile = ref None;;

let argspec =
    [("-o",
      Arg.Set(use_ortc),
      "Use the ORTC metric (RES default)")];;

Arg.parse argspec (fun x -> graphfile := Some(x)) "Usage";;

(* Tree we're constructing *)
type tree_node = NoNode | TreeNode of (int * int * tree_node * tree_node);;

let rec tree_depth (tree : tree_node) : int =
    match tree with
      NoNode -> -1
    | TreeNode(_,_,l,r) -> 1 + (max (tree_depth l) (tree_depth r))
;;

let rec tree_height (tree : tree_node) : int =
    match tree with
      NoNode -> -1
    | TreeNode(_,h,_,_) -> h;
;;

let debug (str : string) : unit =
    (* print_endline str; *)
    ()
;;

type blob = RESblob of Dre.blob | ORTCblob of Ortc.blob;;

let blob_from_set hops nodes =
    if !use_ortc then ORTCblob(Ortc.initial_metric hops nodes)
    else RESblob(Dre.setwise_res hops nodes);;

let score_blob blob =
    match blob with
      ORTCblob(x) -> Ortc.get_metric x
    | RESblob(x) -> Dre.score_res x;;

let combine_blob hops s1 s2 blob1 blob2 =
    match blob1 with
      ORTCblob(x) ->
          (match blob2 with ORTCblob(y) -> ORTCblob(Ortc.combine_metric hops s1 s2 x y)
                            | _ -> raise (Failure "Mismatched blobs"))
    | RESblob(x) ->
          (match blob2 with RESblob(y) -> RESblob(Dre.merge_res hops s1 s2 x y)
                            | _ -> raise (Failure "Mismatched blobs"))


type 'a node_sets_entry = (Dre.nodeset * 'a * tree_node);;
type matrix_entry = (unit -> unit);;

type matrix = matrix_entry option array array;;
type 'a node_sets = 'a node_sets_entry option array;;

type nodepair = (int * int);;

let do_nothing () : unit =
    raise (Failure "do_nothing called")
;;

let (graph,headers) = match !graphfile with
                    Some(x) -> Graph.read_subgraph_file x
                  | None -> Graph.read_subgraph_file "-"
let total_bits =
    match List.filter (fun e -> let (k,_) = e in k = "total-bits") headers with
      e :: [] -> let (k,v) = e in v
   | _ -> 32;;

debug ("Total Bits " ^ (string_of_int total_bits));;
debug ("Graph size " ^ (string_of_int (List.length graph.Graph.nodes)));;
debug ("Edges " ^ (string_of_int (List.length graph.Graph.edges)));;
let hops = Dijkstra.get_all_first_hops graph;;

let size = Array.length hops;;

let estimated_routes = ref (size * (size - 1));;

(* Keep bins of *)
let bins = Mintree.make total_bits;;

let initial_node_set (i : int) (_ : 'b) : 'a node_sets_entry option =
    debug("Setting up node set " ^ (string_of_int i));
    let node_set = Dre.ISet.singleton i in
    Some(node_set,blob_from_set hops node_set,TreeNode(i,0,NoNode,NoNode));;

let node_sets = Array.mapi initial_node_set hops;;

let consider_combining (node_sets : 'a node_sets) (matrix : matrix)
        (hops : int array array) (heap : nodepair Heap.heap)
        (a : int) (b : int) : unit =
    if a == b then raise (Failure "Tried to combine a node with itself");
    let (s1,blob1,_) = match node_sets.(a) with
                      Some(x) -> x
                    | None -> raise (Failure "Bad a") in
    let (s2,blob2,_) = match node_sets.(b) with
                      Some(x) -> x
                    | None -> raise (Failure "Bad b") in
    if a == b then raise (Failure "Tried to combine a node with itself");
    let blob3 = combine_blob hops s1 s2 blob1 blob2 in
    let score = (0 - score_blob blob3) in
    debug ("Adding " ^ (string_of_int score) ^ " (" ^ (string_of_int a) ^ "," ^
        (string_of_int b) ^ ")");
    let remove_func = Heap.insert_remove heap score (a,b) in
    matrix.(a).(b) <- Some(remove_func)
;;

let initialize_heap (node_sets : 'a node_sets) (hops : int array array)
                    (bins : Mintree.bins_t) : (nodepair Heap.heap * matrix) =
    let heap = Heap.make_heap (-1,-1) in
    let (matrix : matrix) = Array.make_matrix size size None in
    debug "init_heap called";
    for i = 0 to size - 1 do
        for j = i + 1 to size - 1 do
            consider_combining node_sets matrix hops heap i j
        done;
        (* Also add to the bins which count how many trees of each
         * depth we have *)
        debug "Calling add_to_bin";
        Mintree.add_to_bin bins 0
    done;
    (heap, matrix)
;;

let remove_from_matrix (matrix : matrix) (a : int) (b : int) : unit =
    for j = 0 to size - 1 do
        match matrix.(a).(j) with
          None -> ()
        | Some(func) -> (debug ("Removing (1) " ^ (string_of_int a) ^ "," ^
            (string_of_int j)); func(); matrix.(a).(j) <- None);
        match matrix.(b).(j) with
          None -> ()
        | Some(func) -> (debug ("Removing (2) " ^ (string_of_int b) ^ "," ^
            (string_of_int j)); func(); matrix.(b).(j) <- None)
    done;
    for i = 0 to size - 1 do
        match matrix.(i).(b) with
          None -> ()
        | Some(func) -> (debug ("Removing (3) " ^ (string_of_int i) ^ "," ^
            (string_of_int b));func(); matrix.(i).(b) <- None);
        match matrix.(i).(a) with
          None -> ()
        | Some(func) -> (debug ("Removing (4) " ^ (string_of_int i) ^ "," ^
            (string_of_int a));func(); matrix.(i).(a) <- None)
    done;
;;

let remove_from_sets (node_sets : 'a node_sets) (a : int) (b : int) : unit =
    node_sets.(a) <- None;
    node_sets.(b) <- None
;;

let combine_with_all (node_sets : 'a node_sets) (matrix : matrix)
        (hops : int array array) (heap : nodepair Heap.heap) (a : int) : unit =
    let combine_with (b : int) : unit =
        let (x,y) = if a < b then (a,b) else (b,a) in
        consider_combining node_sets matrix hops heap x y
    in
    for i = 0 to a - 1 do
        match node_sets.(i) with
          None -> () (* Ignore nodes we've nuked *)
        | Some(_) -> combine_with i
    done;
    for j = a + 1 to size - 1 do
        match node_sets.(j) with
          None -> () (* Ignore nodes we've nuked *)
        | Some(_) -> combine_with j
    done
;;

let combine (node_sets : 'a node_sets) (matrix : matrix)
        (hops : int array array) (heap : nodepair Heap.heap) (a : int)
        (b: int) : unit =
    debug ("Combining " ^ (string_of_int a) ^ " with " ^ (string_of_int b));
    if b <= a then raise (Failure "b <= a");
    let old1 = match node_sets.(a) with
                 None -> raise (Failure "Bad node set entry")
               | Some(x) -> x in
    let old2 = match node_sets.(b) with
                 None -> raise (Failure "Bad node set entry")
               | Some(x) -> x in
    let (s1,blob1,tree1) = old1 in
    let (s2,blob2,tree2) = old2 in
    let h1 = tree_height tree1 in
    let h2 = tree_height tree2 in
    let s3 = Dre.ISet.union s1 s2 in
    let blob3 = combine_blob hops s1 s2 blob1 blob2 in
    let d3 = (1 + max h1 h2) in
    let tree3 = TreeNode(a, d3, tree1, tree2) in
    remove_from_matrix matrix a b;
    (*
            debug "Heap: ";
            Heap.iterw heap (fun x y -> let (a,b) = y in
                debug ((string_of_int x) ^ " = "
                ^ (string_of_int a) ^ ", " ^ (string_of_int b)));
    *)
    remove_from_sets node_sets a b;
    node_sets.(a) <- Some(s3,blob3,tree3);
    combine_with_all node_sets matrix hops heap a
;;

exception OutOfBits;;
let rec greedy_combine (node_sets : 'a node_sets) (matrix : matrix)
        (hops : int array array) (heap : nodepair Heap.heap)
        (remaining : int) (bins: Mintree.bins_t) : tree_node =
    if remaining <= 0 then begin
        let rec find_tree (i : int) : tree_node =
            match node_sets.(i) with
              None -> find_tree (i + 1)
            | Some(_,_,tree) -> tree in
        find_tree 0
    end else begin
        let (score,(a,b)) = Heap.min heap in
        let (x,y) = if a < b then (a,b) else (b,a) in
        estimated_routes := !estimated_routes + score;
        debug ("Combined " ^ (string_of_int x) ^ " and " ^ (string_of_int y));
        (* Try to find the depths of the two subtrees. This is not pretty *)
        match node_sets.(x) with
              None -> debug("X had no node_sets entry")
            | Some(_,_,tree) -> (
                match tree with
                TreeNode(_,h,_,_) -> (
                    debug("X had height " ^ (string_of_int h));
                    Mintree.remove_from_bin bins h
                )
            );
            ;
        match node_sets.(y) with
              None -> debug("Y had no node_sets entry")
            | Some(_,_,tree) -> (
                match tree with
                TreeNode(_,h,_,_) -> (
                    debug("Y had height " ^ (string_of_int h));
                    Mintree.remove_from_bin bins h
                )
            );
            ;
        combine node_sets matrix hops heap x y;
        match node_sets.(x) with
              None -> debug("X has no node_sets entry")
            | Some(_,_,tree) -> (
                match tree with
                TreeNode(_,h,_,_) -> (
                    debug("New x has height " ^ (string_of_int h));
                    Mintree.add_to_bin bins h
                )
            );
            ;
        let min_height = Mintree.height_of bins in
        debug ("Currently uses " ^ (string_of_int min_height) ^
                       " bits (" ^ (string_of_int total_bits) ^ ") bits max");
        if min_height > total_bits then
            raise OutOfBits;
        greedy_combine node_sets matrix hops heap (remaining - 1) bins
    end
;;

let (heap,matrix) = initialize_heap node_sets hops bins in
debug "Heap initialized";
let root = greedy_combine node_sets matrix hops heap (size - 1) bins in
let rec print_tree (root : tree_node) =
    match root with
       NoNode -> ()
    | TreeNode(id,heigt,left,right) -> begin
            print_tree left;
            output_string stdout ((string_of_int id) ^ "\n");
            print_tree right
        end
in
let tree_placement (howmany : int) (tree : tree_node) : (int * int32 array) =
    let locations = Array.make howmany Int32.minus_one in
    let rec helper (tree : tree_node) (depth : int) (sofar : int32) : int =
        (*
        if depth >= 32 then
            raise (Failure "Tree too deep"); *)
        match tree with
          NoNode -> -1
        | TreeNode(id,height,left,right) ->
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
let (depth,placement) = tree_placement (Array.length hops) root in
print_endline ("bits " ^ (string_of_int depth));
print_endline ("routes " ^ (string_of_int !estimated_routes));
Array.iter (fun x -> Printf.printf "%0lu\n" (Int32.shift_right_logical x (32 - depth))) placement
