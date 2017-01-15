import $ from "jquery";

export function startPrerender(prerender, success, fail) {
  return $.ajax({
    type: "POST",
    url: "/prerender",
    dataType: "json",
    contentType: "application/json",
    data: JSON.stringify(prerender),
    success: success,
    error: fail
  });
}

export function loadPrerenderStatus(success, fail) {
  return $.ajax({
    url: "/status",
    success: success,
    error: fail
  });
}
