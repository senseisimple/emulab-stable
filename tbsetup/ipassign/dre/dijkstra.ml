(*
 * dijkstra.ml - Implementation of Dijkstra's shortest-path
 * algorthm
 *)

type ('a,'b) dijk_state = {
    graph : ('a, 'b) Graph.t;
    visited : bool array; (* Not yet used - ditch? *)
    estimate : 'b array;
    pred : ('a, 'b) Graph.node array;
    heap : ('a, 'b) Graph.node Heap.heap;
    updaters : (int -> unit) array;
};;

let init (graph : ('a,'b) Graph.t) (node : ('a,'b) Graph.node)
        : ('a,'b) dijk_state =
    let size = Graph.size graph in
    let state = { graph = graph;
      visited = Array.make size false;
      estimate = Array.make size 32000;
      (* XXX - Bad! I have to pick a type for the node contents graph *)
      pred = Array.make size (Graph.empty_node 0);
      heap = Heap.make_heap (Graph.empty_node 0);
      updaters = Array.make size (fun (a:int) -> ()) } in
    state.estimate.(node.Graph.node_contents) <- 0;
    let rec fill_heap (nodes : ('a, 'b) Graph.node list) : unit =
        match nodes with
          [] -> ()
        | x::xs ->
                let node_id = x.Graph.node_contents in
                state.updaters.(node_id) <-
                    Heap.insert state.heap state.estimate.(node_id) x;
                fill_heap xs
    in fill_heap graph.Graph.nodes;
    state
;;

let relax (state : ('a, 'b) dijk_state) (u : ('a, 'b) Graph.node)
          (v : ('a, 'b) Graph.node) (edge : ('a, 'b) Graph.edge) : unit =
    let uid = u.Graph.node_contents in
    let vid = v.Graph.node_contents in
    let followed_weight =
        (state.estimate.(uid) + edge.Graph.contents) in
    (* print_endline ("relax: uid=" ^ string_of_int uid ^ " vid="
    ^ string_of_int vid ^ " fw=" ^ string_of_int followed_weight); *)
    if state.estimate.(vid) > followed_weight then
        (state.estimate.(vid) <- followed_weight;
        state.updaters.(vid) followed_weight;
        state.pred.(vid) <- u)
    else
        ()
;;

let rec process_edges (state : ('a, 'b) dijk_state) (node : ('a,'b) Graph.node)
        (edges : ('a, 'b) Graph.edge list) : unit =
    match edges with
      [] -> ()
    | edge::rest -> (
        let dst = if edge.Graph.src != node then edge.Graph.src
                                            else edge.Graph.dst in
        if (state.visited.(dst.Graph.node_contents)) then () else
        relax state node dst edge;
        process_edges state node rest)
;;

let rec process_nodes (state : ('a, 'b) dijk_state) : unit =
    try (match Heap.min state.heap with (weight,node) ->
        Heap.extract_min state.heap;
        state.visited.(node.Graph.node_contents) <- true;
        process_edges state node node.Graph.node_edges;
        process_nodes state)
    with Heap.EmptyHeap -> ()
;;

let run_dijkstra (graph : ('a, 'b) Graph.t) (node : ('a, 'b) Graph.node)
        : ('b array * ('a, 'b) Graph.node array) =
    let state = init graph node in
    process_nodes state;
    (state.estimate, state.pred)
;;

type ('a,'b) internal_first_hop = INoHop | INoHopYet | INodeHop of ('a,'b) Graph.node;;
type ('a,'b) first_hop = NoHop | NodeHop of ('a,'b) Graph.node;;
let string_of_fh (fh : (int, 'b) first_hop) : string =
    match fh with
      NoHop -> "NoHop"
    | NodeHop(n) -> string_of_int n.Graph.node_contents
;;
(* XXX - there has got to be a better way to do this, I'm sure *)
let fh_equal (a : ('a, 'b) first_hop) (b : ('a, 'b) first_hop) : bool =
    match a with
      NoHop ->
          (match b with NoHop -> true | _ -> false)
    | NodeHop(x) ->
            (match b with
               NoHop -> false
             | NodeHop(y) -> x.Graph.node_contents == y.Graph.node_contents)
;;

exception HopInternalError;;
let get_first_hops (graph : ('a, 'b) Graph.t)
                   (pred : ('a, 'b) Graph.node array)
                   (root : ('a, 'b) Graph.node) : ('a, 'b) first_hop array =
    let hops = Array.make (Array.length pred) INoHopYet in
    let out_hops = Array.make (Array.length pred) NoHop in
    hops.(root.Graph.node_contents) <- INoHop;
    let rec hop_helper (node : ('a, 'b) Graph.node) : ('a, 'b) internal_first_hop =
        match hops.(node.Graph.node_contents) with
          INoHop -> INoHop (* The root has no first hop *)
        | INodeHop(hop) -> INodeHop(hop) (* We've already found the first hop *)
        | INoHopYet ->
               (let parent = pred.(node.Graph.node_contents) in
                let hop = if parent.Graph.node_contents == root.Graph.node_contents
                    then INodeHop(node) else hop_helper parent in
                hops.(node.Graph.node_contents) <- hop;
                hop)
    in
    let rec all_hops (nodes : ('a, 'b) Graph.node list) : unit =
        match nodes with
          [] -> ()
        | x :: xs -> let _ = hop_helper x in all_hops xs
    in
    let rec copy_hops (i : int) : unit =
        if i >= Array.length hops then () else
            begin
                (match hops.(i) with
                  INoHop -> out_hops.(i) <- NoHop
                | INodeHop(h) -> out_hops.(i) <- NodeHop(h)
                | INoHopYet -> raise HopInternalError);
                copy_hops (i+1)
            end
            
    in
    all_hops graph.Graph.nodes;
    copy_hops 0;
    out_hops
;;

(*
let get_all_first_hops (graph : ('a, 'b) Graph.t)
                       (pred : ('a, 'b) Graph.node array)
                     : ('a, 'b) first_hop array array =
    let all_hops = Array.make_matrix (Graph.count_nodes graph) 0 NoHop in
    let fill_array (base : unit) (node : (int, 'a) Graph.node) : unit =
        let node_id = node.Graph.node_contents in
        all_hops.(node_id) <- get_first_hops graph pred node;
        base
    in
    Graph.fold_nodes graph fill_array ();
    all_hops
;;
*)
